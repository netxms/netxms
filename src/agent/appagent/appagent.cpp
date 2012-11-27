/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: appagent.cpp
**
**/

#include "appagent-internal.h"

/**
 * Application agent config
 */
static APPAGENT_INIT s_config;
static bool s_stop = false;
static bool s_initialized = false;
static HPIPE s_pipe = INVALID_PIPE_HANDLE;
static THREAD s_connectorThread = INVALID_THREAD_HANDLE;

/**
 * Write log
 */
static void AppAgentWriteLog(int level, const TCHAR *format, ...)
{
	if (s_config.logger == NULL)
		return;

	va_list(args);
	va_start(args, format);
	s_config.logger(level, format, args);
	va_end(args);
}

/**
 * Get metric
 */
static APPAGENT_MSG *GetMetric(WCHAR *name, int length)
{
	TCHAR metricName[256];

#ifdef UNICODE
	nx_strncpy(metricName, name, min(length, 256));
#else
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, name, length, metricName, 256, NULL, NULL);
	metricName[min(length, 255)] = 0;
#endif

	for(int i = 0; i < s_config.numMetrics; i++)
	{
		if (MatchString(s_config.metrics[i].name, metricName, FALSE))
		{
			TCHAR value[256];
			int rcc = s_config.metrics[i].handler(metricName, s_config.metrics[i].userArg, value);

			APPAGENT_MSG *msg;
			if (rcc == APPAGENT_RCC_SUCCESS)
			{
				int len = (int)_tcslen(value) + 1;
				msg = NewMessage(APPAGENT_CMD_REQUEST_COMPLETED, rcc, len * sizeof(WCHAR));
#ifdef UNICODE
				wcscpy((WCHAR *)msg->payload, value);
#else
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, value, -1, (WCHAR *)msg->payload, len);
#endif
			}
			else
			{
				msg = NewMessage(APPAGENT_CMD_REQUEST_COMPLETED, rcc, 0);
			}
			return msg;
		}
	}
	return NewMessage(APPAGENT_CMD_REQUEST_COMPLETED, APPAGENT_RCC_NO_SUCH_METRIC, 0);
}

/**
 * Process incoming request
 */
static void ProcessRequest(HPIPE hPipe)
{
	MessageBuffer *mb = new MessageBuffer;

	AppAgentWriteLog(5, _T("ProcessRequest: connection established"));
	while(true)
	{
		APPAGENT_MSG *msg = ReadMessageFromPipe(hPipe, NULL, mb);
		if (msg == NULL)
			break;
		AppAgentWriteLog(6, _T("ProcessRequest: received message %04X"), (unsigned int)msg->command);
		APPAGENT_MSG *response;
		switch(msg->command)
		{
			case APPAGENT_CMD_GET_METRIC:
				response = GetMetric((WCHAR *)msg->payload, msg->length - APPAGENT_MSG_HEADER_LEN);
				break;
			default:
				response = NewMessage(APPAGENT_CMD_REQUEST_COMPLETED, APPAGENT_RCC_BAD_REQUEST, 0);
				break;
		}
		free(msg);
		SendMessageToPipe(hPipe, response);
		free(response);
	}
	AppAgentWriteLog(5, _T("ProcessRequest: connection closed"));
	delete mb;
}

/**
 * Connector thread for external push command
 */
#ifdef _WIN32

static THREAD_RESULT THREAD_CALL AppAgentConnector(void *arg)
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
		AppAgentWriteLog(2, _T("AppAgentConnector: AllocateAndInitializeSid failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow either Everyone or given user to access pipe
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = (FILE_GENERIC_READ | FILE_GENERIC_WRITE) & ~FILE_CREATE_PIPE_INSTANCE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	if ((s_config.userId == NULL) || (s_config.userId[0] == 0) || !_tcscmp(s_config.userId, _T("*")))
	{
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName  = (LPTSTR)sidEveryone;
	}
	else
	{
		ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
		ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea.Trustee.ptstrName  = (LPTSTR)s_config.userId;
		AppAgentWriteLog(2, _T("AppAgentConnector: will allow connections only for user %s"), s_config.userId);
	}

	// Create a new ACL that contains the new ACEs.
	if (SetEntriesInAcl(1, &ea, NULL, &acl) != ERROR_SUCCESS)
	{
		AppAgentWriteLog(2, _T("AppAgentConnector: SetEntriesInAcl failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (sd == NULL)
	{
		AppAgentWriteLog(2, _T("AppAgentConnector: LocalAlloc failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
	{
		AppAgentWriteLog(2, _T("AppAgentConnector: InitializeSecurityDescriptor failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Add the ACL to the security descriptor. 
   if (!SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE))
	{
		AppAgentWriteLog(2, _T("AppAgentConnector: SetSecurityDescriptorDacl failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	TCHAR pipeName[MAX_PATH];
	_sntprintf(pipeName, MAX_PATH, _T("\\\\.\\pipe\\appagent.%s"), s_config.name);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = sd;
	s_pipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 8192, 8192, 0, &sa);
	if (s_pipe == INVALID_HANDLE_VALUE)
	{
		AppAgentWriteLog(2, _T("AppAgentConnector: CreateNamedPipe failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	AppAgentWriteLog(2, _T("AppAgentConnector: named pipe created, waiting for connection"));
	int connectErrors = 0;
	while(!s_stop)
	{
		BOOL connected = ConnectNamedPipe(s_pipe, NULL);
		if (connected || (GetLastError() == ERROR_PIPE_CONNECTED))
		{
			ProcessRequest(s_pipe);
			DisconnectNamedPipe(s_pipe);
			connectErrors = 0;
		}
		else
		{
			AppAgentWriteLog(2, _T("AppAgentConnector: ConnectNamedPipe failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
			connectErrors++;
			if (connectErrors > 10)
				break;	// Stop this connector if ConnectNamedPipe fails instantly
		}
	}

cleanup:
	if (s_pipe != NULL)
	{
		CloseHandle(s_pipe);
		s_pipe = INVALID_PIPE_HANDLE;
	}

	if (sd != NULL)
		LocalFree(sd);

	if (acl != NULL)
		LocalFree(acl);

	if (sidEveryone != NULL)
		FreeSid(sidEveryone);

	AppAgentWriteLog(2, _T("AppAgentConnector: listener thread stopped"));
	return THREAD_OK;
}

#else

static THREAD_RESULT THREAD_CALL AppAgentConnector(void *arg)
{
	mode_t prevMask = 0;

	s_pipe = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s_pipe == -1)
	{
		AppAgentWriteLog(2, _T("AppAgentConnector: socket failed (%s)"), _tcserror(errno));
		goto cleanup;
	}
	
	struct sockaddr_un addrLocal;
	addrLocal.sun_family = AF_UNIX;
	sprintf(addrLocal.sun_path, "/tmp/.appagent.%s", s_config.name);	
	unlink(addrLocal.sun_path);
	prevMask = umask(S_IWGRP | S_IWOTH);
	if (bind(s_pipe, (struct sockaddr *)&addrLocal, SUN_LEN(&addrLocal)) == -1)
	{
		AppAgentWriteLog(2, _T("AppAgentConnector: bind failed (%s)"), _tcserror(errno));
		umask(prevMask);
		goto cleanup;
	}
	umask(prevMask);

	if (listen(s_pipe, 5) == -1)
	{
		AppAgentWriteLog(2, _T("AppAgentConnector: listen failed (%s)"), _tcserror(errno));
		goto cleanup;
	}
	
	while(!s_stop)
	{
		struct sockaddr_un addrRemote;
		socklen_t size = sizeof(struct sockaddr_un);
		SOCKET cs = accept(s_pipe, (struct sockaddr *)&addrRemote, &size);
		if (cs > 0)
		{
			ProcessRequest(cs);
			shutdown(cs, 2);
			close(cs);
		}
		else
		{
			AppAgentWriteLog(2, _T("AppAgentConnector: accept failed (%s)"), _tcserror(errno));
		}
	}

cleanup:
	if (s_pipe != -1)
	{
		close(s_pipe);
		s_pipe = INVALID_PIPE_HANDLE;
	}

	AppAgentWriteLog(2, _T("AppAgentConnector: listener thread stopped"));
	return THREAD_OK;
}

#endif

/**
 * Initialize application agent
 */
bool APPAGENT_EXPORTABLE AppAgentInit(APPAGENT_INIT *initData)
{
	if (s_initialized)
		return false;	// already initialized

	memcpy(&s_config, initData, sizeof(APPAGENT_INIT));
	if ((s_config.name == NULL) || (s_config.name[0] == 0))
		return false;
	s_initialized = true;
	return true;
}

/**
 * Start application agent
 */
void APPAGENT_EXPORTABLE AppAgentStart()
{
	if ((s_initialized) && (s_connectorThread == INVALID_THREAD_HANDLE))
		s_connectorThread = ThreadCreateEx(AppAgentConnector, 0, NULL);
}

/**
 * Stop application agent
 */
void APPAGENT_EXPORTABLE AppAgentStop()
{
	if (s_initialized && (s_connectorThread != INVALID_THREAD_HANDLE))
	{
		s_stop = true;
		if (s_pipe != INVALID_THREAD_HANDLE)
		{
#ifdef _WIN32
			CloseHandle(s_pipe);
#else
			shutdown(s_pipe, SHUT_RDWR);
#endif
			s_pipe = INVALID_PIPE_HANDLE;
		}
		ThreadJoin(s_connectorThread);
		s_connectorThread = INVALID_THREAD_HANDLE;
	}
}
