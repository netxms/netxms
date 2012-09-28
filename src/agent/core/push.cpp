/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2012 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: push.cpp
**
**/

#include "nxagentd.h"


/**
 * Push parameter's data
 */
BOOL PushData(const TCHAR *parameter, const TCHAR *value)
{
	CSCPMessage msg;
	BOOL success = FALSE;

	AgentWriteDebugLog(6, _T("PushData: \"%s\" = \"%s\""), parameter, value);

	msg.SetCode(CMD_PUSH_DCI_DATA);
	msg.SetVariable(VID_NAME, parameter);
	msg.SetVariable(VID_VALUE, value);

   MutexLock(g_hSessionListAccess);
   for(DWORD i = 0; i < g_dwMaxSessions; i++)
      if (g_pSessionList[i] != NULL)
         if (g_pSessionList[i]->canAcceptTraps())
         {
            g_pSessionList[i]->sendMessage(&msg);
            success = TRUE;
         }
   MutexUnlock(g_hSessionListAccess);

	return success;
}

/**
 * Process push request
 */
static void ProcessPushRequest(HPIPE hPipe)
{
	TCHAR buffer[256];

	AgentWriteDebugLog(5, _T("ProcessPushRequest: connection established"));
	while(true)
	{
		CSCPMessage *msg = ReadMessageFromPipe(hPipe, NULL);
		if (msg == NULL)
			break;
		AgentWriteDebugLog(6, _T("ProcessPushRequest: received message %s"), NXCPMessageCodeName(msg->GetCode(), buffer));
		if (msg->GetCode() == CMD_PUSH_DCI_DATA)
		{
			DWORD count = msg->GetVariableLong(VID_NUM_ITEMS);
			DWORD varId = VID_PUSH_DCI_DATA_BASE;
			for(DWORD i = 0; i < count; i++)
			{
				TCHAR name[MAX_PARAM_NAME], value[MAX_RESULT_LENGTH];
				msg->GetVariableStr(varId++, name, MAX_PARAM_NAME);
				msg->GetVariableStr(varId++, value, MAX_RESULT_LENGTH);
				PushData(name, value);
			}
		}
		else
		{
		}
		delete msg;
	}
	AgentWriteDebugLog(5, _T("ProcessPushRequest: connection closed"));
}

/**
 * Connector thread for external push command
 */
#ifdef _WIN32

static THREAD_RESULT THREAD_CALL PushConnector(void *arg)
{
	SECURITY_ATTRIBUTES sa;
	PSECURITY_DESCRIPTOR sd = NULL;
	SID_IDENTIFIER_AUTHORITY sidAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	EXPLICIT_ACCESS ea;
	PSID sidEveryone = NULL;
	ACL *acl = NULL;
	TCHAR errorText[1024];

	// Create a well-known SID for the Everyone group.
	if(!AllocateAndInitializeSid(&sidAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &sidEveryone))
	{
		AgentWriteDebugLog(2, _T("PushConnector: AllocateAndInitializeSid failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow either Everyone or given user to access pipe
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = (FILE_GENERIC_READ | FILE_GENERIC_WRITE) & ~FILE_CREATE_PIPE_INSTANCE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	const TCHAR *user = g_config->getValue(_T("/agent/PushUser"), _T("*"));
	if ((user[0] == 0) || !_tcscmp(user, _T("*")))
	{
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName  = (LPTSTR)sidEveryone;
	}
	else
	{
		ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
		ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea.Trustee.ptstrName  = (LPTSTR)user;
		AgentWriteDebugLog(2, _T("PushConnector: will allow connections only for user %s"), user);
	}

	// Create a new ACL that contains the new ACEs.
	if (SetEntriesInAcl(1, &ea, NULL, &acl) != ERROR_SUCCESS)
	{
		AgentWriteDebugLog(2, _T("PushConnector: SetEntriesInAcl failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (sd == NULL)
	{
		AgentWriteDebugLog(2, _T("PushConnector: LocalAlloc failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
	{
		AgentWriteDebugLog(2, _T("PushConnector: InitializeSecurityDescriptor failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Add the ACL to the security descriptor. 
   if (!SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE))
	{
		AgentWriteDebugLog(2, _T("PushConnector: SetSecurityDescriptorDacl failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = sd;
	HANDLE hPipe = CreateNamedPipe(_T("\\\\.\\pipe\\nxagentd.push"), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 8192, 8192, 0, &sa);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		AgentWriteDebugLog(2, _T("PushConnector: CreateNamedPipe failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	AgentWriteDebugLog(2, _T("PushConnector: named pipe created, waiting for connection"));
	int connectErrors = 0;
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
		BOOL connected = ConnectNamedPipe(hPipe, NULL);
		if (connected || (GetLastError() == ERROR_PIPE_CONNECTED))
		{
			ProcessPushRequest(hPipe);
			DisconnectNamedPipe(hPipe);
			connectErrors = 0;
		}
		else
		{
			AgentWriteDebugLog(2, _T("PushConnector: ConnectNamedPipe failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
			connectErrors++;
			if (connectErrors > 10)
				break;	// Stop this connector if ConnectNamedPipe fails instantly
		}
	}

cleanup:
	if (hPipe != NULL)
		CloseHandle(hPipe);

	if (sd != NULL)
		LocalFree(sd);

	if (acl != NULL)
		LocalFree(acl);

	if (sidEveryone != NULL)
		FreeSid(sidEveryone);

	AgentWriteDebugLog(2, _T("PushConnector: listener thread stopped"));
	return THREAD_OK;
}

#else

static THREAD_RESULT THREAD_CALL PushConnector(void *arg)
{
	mode_t prevMask = 0;

	SOCKET hPipe = socket(AF_UNIX, SOCK_STREAM, 0);
	if (hPipe == -1)
	{
		AgentWriteDebugLog(2, _T("PushConnector: socket failed (%s)"), _tcserror(errno));
		goto cleanup;
	}
	
	struct sockaddr_un addrLocal;
	addrLocal.sun_family = AF_UNIX;
	strcpy(addrLocal.sun_path, "/tmp/.nxagentd.push");	
	unlink(addrLocal.sun_path);
	prevMask = umask(S_IWGRP | S_IWOTH);
	if (bind(hPipe, (struct sockaddr *)&addrLocal, SUN_LEN(&addrLocal)) == -1)
	{
		AgentWriteDebugLog(2, _T("PushConnector: bind failed (%s)"), _tcserror(errno));
		umask(prevMask);
		goto cleanup;
	}
	umask(prevMask);

	if (listen(hPipe, 5) == -1)
	{
		AgentWriteDebugLog(2, _T("PushConnector: listen failed (%s)"), _tcserror(errno));
		goto cleanup;
	}
	
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
		struct sockaddr_un addrRemote;
		socklen_t size = sizeof(struct sockaddr_un);
		SOCKET cs = accept(hPipe, (struct sockaddr *)&addrRemote, &size);
		if (cs > 0)
		{
			ProcessPushRequest(cs);
			shutdown(cs, 2);
			close(cs);
		}
		else
		{
			AgentWriteDebugLog(2, _T("PushConnector: accept failed (%s)"), _tcserror(errno));
		}
	}

cleanup:
	if (hPipe != -1)
		close(hPipe);

	AgentWriteDebugLog(2, _T("PushConnector: listener thread stopped"));
	return THREAD_OK;
}

#endif

/**
 * Start push connector
 */

void StartPushConnector()
{
	ThreadCreate(PushConnector, 0, NULL);
}
