/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2011 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: extagent.cpp
**
**/

#include "nxagentd.h"

#ifdef _WIN32
#include <aclapi.h>
#endif


//
// Static data
//

static ObjectArray<ExternalSubagent> s_subagents;


//
// Constructor
//

ExternalSubagent::ExternalSubagent(const TCHAR *name)
{
	nx_strncpy(m_name, name, MAX_SUBAGENT_NAME);
	m_connected = false;
	m_pipe = NULL;
}


//
// Destructor
//

ExternalSubagent::~ExternalSubagent()
{
}


//
// Read NXCP message from pipe
//

static CSCPMessage *ReadMessageFromPipe(HANDLE hPipe)
{
	BYTE buffer[8192];
	DWORD bytes;

	if (!ReadFile(hPipe, buffer, 8192, &bytes, NULL))
		return NULL;

	if (bytes < CSCP_HEADER_SIZE)
		return NULL;

	if (ntohl(((CSCP_MESSAGE *)buffer)->dwSize) != bytes)
		return NULL;	// message size given in header does not match number of received bytes

	return new CSCPMessage((CSCP_MESSAGE *)buffer);
}


//
// Main connection thread
//

void ExternalSubagent::connect(HANDLE hPipe)
{
	TCHAR buffer[256];

	m_pipe = hPipe;
	m_connected = true;

	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): connection established"), m_name);
	while(true)
	{
		CSCPMessage *msg = ReadMessageFromPipe(hPipe);
		if (msg == NULL)
			break;
		AgentWriteDebugLog(6, _T("ExternalSubagent(%s): received message %s"), m_name, NXCPMessageCodeName(msg->GetCode(), buffer));

		delete msg;
	}

	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): connection closed"), m_name);
	m_connected = false;
}


//
// Listener for external subagents
//

static THREAD_RESULT THREAD_CALL ExternalSubagentConnector(void *arg)
{
	ExternalSubagent *subagent = (ExternalSubagent *)arg;
	SECURITY_ATTRIBUTES sa;
	PSECURITY_DESCRIPTOR sd = NULL;
	SID_IDENTIFIER_AUTHORITY sidAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	EXPLICIT_ACCESS ea;
	PSID sidEveryone = NULL;
	ACL *acl = NULL;
	TCHAR pipeName[MAX_PATH], errorText[1024];

	// Create a well-known SID for the Everyone group.
	if(!AllocateAndInitializeSid(&sidAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &sidEveryone))
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: AllocateAndInitializeSid failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow Everyone write access to the key.
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = (FILE_GENERIC_READ | FILE_GENERIC_WRITE) & ~FILE_CREATE_PIPE_INSTANCE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName  = (LPTSTR)sidEveryone;

	// Create a new ACL that contains the new ACEs.
	if (SetEntriesInAcl(1, &ea, NULL, &acl) != ERROR_SUCCESS)
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: SetEntriesInAcl failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (sd == NULL)
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: LocalAlloc failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: InitializeSecurityDescriptor failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Add the ACL to the security descriptor. 
   if (!SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE))
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: SetSecurityDescriptorDacl failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = sd;
	_sntprintf(pipeName, MAX_PATH, _T("\\\\.\\pipe\\nxagentd.subagent.%s"), subagent->getName());
	HANDLE hPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 8192, 8192, 0, &sa);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: CreateNamedPipe failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): named pipe created, waiting for connection"), subagent->getName());
	int connectErrors = 0;
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
		BOOL connected = ConnectNamedPipe(hPipe, NULL);
		if (connected || (GetLastError() == ERROR_PIPE_CONNECTED))
		{
			subagent->connect(hPipe);
			DisconnectNamedPipe(hPipe);
			connectErrors = 0;
		}
		else
		{
			AgentWriteDebugLog(2, _T("ExternalSubagentConnector: ConnectNamedPipe failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
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

	return THREAD_OK;
}


//
// Add external subagent from config
//

bool AddExternalSubagent(const TCHAR *config)
{
	ExternalSubagent *subagent = new ExternalSubagent(config);
	s_subagents.add(subagent);
	ThreadCreate(ExternalSubagentConnector, 0, subagent);
	return true;
}
