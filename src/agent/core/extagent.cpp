/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2013 Victor Kirhenshtein
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


/**
 *Static data
 */
static ObjectArray<ExternalSubagent> s_subagents;

/**
 * Constructor
 */
ExternalSubagent::ExternalSubagent(const TCHAR *name, const TCHAR *user)
{
	nx_strncpy(m_name, name, MAX_SUBAGENT_NAME);
	nx_strncpy(m_user, user, MAX_ESA_USER_NAME);
	m_connected = false;
	m_pipe = INVALID_PIPE_HANDLE;
	m_msgQueue = new MsgWaitQueue();
	m_requestId = 1;
	m_mutexPipeWrite = MutexCreate();
}

/**
 * Destructor
 */
ExternalSubagent::~ExternalSubagent()
{
	delete m_msgQueue;
	MutexDestroy(m_mutexPipeWrite);
}

/*
 * Send message to external subagent
 */
bool ExternalSubagent::sendMessage(NXCPMessage *msg)
{
	TCHAR buffer[256];
	AgentWriteDebugLog(6, _T("ExternalSubagent::sendMessage(%s): sending message %s"), m_name, NXCPMessageCodeName(msg->getCode(), buffer));

	NXCP_MESSAGE *rawMsg = msg->createMessage();
	MutexLock(m_mutexPipeWrite);
   bool success = SendMessageToPipe(m_pipe, rawMsg);
	MutexUnlock(m_mutexPipeWrite);
	free(rawMsg);
	return success;
}

/**
 * Wait for specific message to arrive
 */
NXCPMessage *ExternalSubagent::waitForMessage(WORD code, UINT32 id)
{
	return m_msgQueue->waitForMessage(code, id, 5000);	// 5 sec timeout
}

/**
 * Main connection thread
 */
void ExternalSubagent::connect(HPIPE hPipe)
{
	TCHAR buffer[256];
   UINT32 i;

	m_pipe = hPipe;
	m_connected = true;
	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): connection established"), m_name);
   PipeMessageReceiver receiver(hPipe, 8192, 1048576);  // 8K initial, 1M max
	while(true)
	{
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(INFINITE, &result);
		if (msg == NULL)
      {
         AgentWriteDebugLog(6, _T("ExternalSubagent(%s): receiver failure (%s)"), m_name, AbstractMessageReceiver::resultToText(result));
			break;
      }
		AgentWriteDebugLog(6, _T("ExternalSubagent(%s): received message %s"), m_name, NXCPMessageCodeName(msg->getCode(), buffer));
      switch(msg->getCode())
      {
         case CMD_PUSH_DCI_DATA:
            MutexLock(g_hSessionListAccess);
            for(i = 0; i < g_dwMaxSessions; i++)
               if (g_pSessionList[i] != NULL)
                  if (g_pSessionList[i]->canAcceptTraps())
                  {
                     g_pSessionList[i]->sendMessage(msg);
                  }
            MutexUnlock(g_hSessionListAccess);
            delete msg;
            break;
         case CMD_TRAP:
            ForwardTrap(msg);
            delete msg;
            break;
         default:
		      m_msgQueue->put(msg);
            break;
      }
	}

	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): connection closed"), m_name);
	m_connected = false;
	m_msgQueue->clear();
}

/**
 * Send shutdown command
 */
void ExternalSubagent::shutdown()
{
	NXCPMessage msg;
	msg.setCode(CMD_SHUTDOWN);
	msg.setId(m_requestId++);
	sendMessage(&msg);
}

/**
 * Get list of supported parameters
 */
NETXMS_SUBAGENT_PARAM *ExternalSubagent::getSupportedParameters(UINT32 *count)
{
	NXCPMessage msg;
	NETXMS_SUBAGENT_PARAM *result = NULL;

	msg.setCode(CMD_GET_PARAMETER_LIST);
	msg.setId(m_requestId++);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != NULL)
		{
			if (response->getFieldAsUInt32(VID_RCC) == ERR_SUCCESS)
			{
				*count = response->getFieldAsUInt32(VID_NUM_PARAMETERS);
				result = (NETXMS_SUBAGENT_PARAM *)malloc(*count * sizeof(NETXMS_SUBAGENT_PARAM));
				UINT32 varId = VID_PARAM_LIST_BASE;
				for(UINT32 i = 0; i < *count; i++)
				{
					response->getFieldAsString(varId++, result[i].name, MAX_PARAM_NAME);
					response->getFieldAsString(varId++, result[i].description, MAX_DB_STRING);
					result[i].dataType = (int)response->getFieldAsUInt16(varId++);
				}
			}
			delete response;
		}
	}
	return result;
}

/**
 * Get list of supported lists
 */
NETXMS_SUBAGENT_LIST *ExternalSubagent::getSupportedLists(UINT32 *count)
{
	NXCPMessage msg;
	NETXMS_SUBAGENT_LIST *result = NULL;

	msg.setCode(CMD_GET_ENUM_LIST);
	msg.setId(m_requestId++);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != NULL)
		{
			if (response->getFieldAsUInt32(VID_RCC) == ERR_SUCCESS)
			{
				*count = response->getFieldAsUInt32(VID_NUM_ENUMS);
				result = (NETXMS_SUBAGENT_LIST *)malloc(*count * sizeof(NETXMS_SUBAGENT_LIST));
				UINT32 varId = VID_ENUM_LIST_BASE;
				for(UINT32 i = 0; i < *count; i++)
				{
					response->getFieldAsString(varId++, result[i].name, MAX_PARAM_NAME);
				}
			}
			delete response;
		}
	}
	return result;
}

/**
 * Get list of supported tables
 */
NETXMS_SUBAGENT_TABLE *ExternalSubagent::getSupportedTables(UINT32 *count)
{
	NXCPMessage msg;
	NETXMS_SUBAGENT_TABLE *result = NULL;

	msg.setCode(CMD_GET_TABLE_LIST);
	msg.setId(m_requestId++);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != NULL)
		{
			if (response->getFieldAsUInt32(VID_RCC) == ERR_SUCCESS)
			{
				*count = response->getFieldAsUInt32(VID_NUM_TABLES);
				result = (NETXMS_SUBAGENT_TABLE *)malloc(*count * sizeof(NETXMS_SUBAGENT_TABLE));
				UINT32 varId = VID_TABLE_LIST_BASE;
				for(UINT32 i = 0; i < *count; i++)
				{
					response->getFieldAsString(varId++, result[i].name, MAX_PARAM_NAME);
					response->getFieldAsString(varId++, result[i].instanceColumns, MAX_COLUMN_NAME * MAX_INSTANCE_COLUMNS);
					response->getFieldAsString(varId++, result[i].description, MAX_DB_STRING);
				}
			}
			delete response;
		}
	}
	return result;
}

/**
 * List supported parameters
 */
void ExternalSubagent::listParameters(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_PARAM *list = getSupportedParameters(&paramCount);
	if (list != NULL)
	{
		UINT32 id = *baseId;

		for(UINT32 i = 0; i < paramCount; i++)
		{
			msg->setField(id++, list[i].name);
			msg->setField(id++, list[i].description);
			msg->setField(id++, (WORD)list[i].dataType);
		}
		*baseId = id;
		*count += paramCount;
		free(list);
	}
}

/**
 * List supported parameters
 */
void ExternalSubagent::listParameters(StringList *list)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_PARAM *plist = getSupportedParameters(&paramCount);
	if (plist != NULL)
	{
		for(UINT32 i = 0; i < paramCount; i++)
			list->add(plist[i].name);
		free(plist);
	}
}

/**
 * List supported lists
 */
void ExternalSubagent::listLists(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_LIST *list = getSupportedLists(&paramCount);
	if (list != NULL)
	{
		UINT32 id = *baseId;

		for(UINT32 i = 0; i < paramCount; i++)
		{
			msg->setField(id++, list[i].name);
		}
		*baseId = id;
		*count += paramCount;
		free(list);
	}
}

/**
 * List supported lists
 */
void ExternalSubagent::listLists(StringList *list)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_LIST *plist = getSupportedLists(&paramCount);
	if (plist != NULL)
	{
		for(UINT32 i = 0; i < paramCount; i++)
			list->add(plist[i].name);
		free(plist);
	}
}

/**
 * List supported tables
 */
void ExternalSubagent::listTables(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_TABLE *list = getSupportedTables(&paramCount);
	if (list != NULL)
	{
		UINT32 id = *baseId;

		for(UINT32 i = 0; i < paramCount; i++)
		{
			msg->setField(id++, list[i].name);
			msg->setField(id++, list[i].instanceColumns);
			msg->setField(id++, list[i].description);
		}
		*baseId = id;
		*count += paramCount;
		free(list);
	}
}

/**
 * List supported tables
 */
void ExternalSubagent::listTables(StringList *list)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_TABLE *plist = getSupportedTables(&paramCount);
	if (plist != NULL)
	{
		for(UINT32 i = 0; i < paramCount; i++)
			list->add(plist[i].name);
		free(plist);
	}
}

/**
 * Get parameter value from external subagent
 */
UINT32 ExternalSubagent::getParameter(const TCHAR *name, TCHAR *buffer)
{
	NXCPMessage msg;
	UINT32 rcc;

	msg.setCode(CMD_GET_PARAMETER);
	msg.setId(m_requestId++);
	msg.setField(VID_PARAMETER, name);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != NULL)
		{
			rcc = response->getFieldAsUInt32(VID_RCC);
			if (rcc == ERR_SUCCESS)
				response->getFieldAsString(VID_VALUE, buffer, MAX_RESULT_LENGTH);
			delete response;
		}
		else
		{
			rcc = ERR_INTERNAL_ERROR;
		}
	}
	else
	{
		rcc = ERR_CONNECTION_BROKEN;
	}
	return rcc;
}

/**
 * Get table value from external subagent
 */
UINT32 ExternalSubagent::getTable(const TCHAR *name, Table *value)
{
	NXCPMessage msg;
	UINT32 rcc;

	msg.setCode(CMD_GET_TABLE);
	msg.setId(m_requestId++);
	msg.setField(VID_PARAMETER, name);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != NULL)
		{
			rcc = response->getFieldAsUInt32(VID_RCC);
			if (rcc == ERR_SUCCESS)
				value->updateFromMessage(response);
			delete response;
		}
		else
		{
			rcc = ERR_INTERNAL_ERROR;
		}
	}
	else
	{
		rcc = ERR_CONNECTION_BROKEN;
	}
	return rcc;
}

/**
 * Get list value from external subagent
 */
UINT32 ExternalSubagent::getList(const TCHAR *name, StringList *value)
{
	NXCPMessage msg;
	UINT32 rcc;

	msg.setCode(CMD_GET_LIST);
	msg.setId(m_requestId++);
	msg.setField(VID_PARAMETER, name);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != NULL)
		{
			rcc = response->getFieldAsUInt32(VID_RCC);
			if (rcc == ERR_SUCCESS)
			{
            UINT32 count = response->getFieldAsUInt32(VID_NUM_STRINGS);
            for(UINT32 i = 0; i < count; i++)
					value->addPreallocated(response->getFieldAsString(VID_ENUM_VALUE_BASE + i));
			}
			delete response;
		}
		else
		{
			rcc = ERR_INTERNAL_ERROR;
		}
	}
	else
	{
		rcc = ERR_CONNECTION_BROKEN;
	}
	return rcc;
}

/**
 * Listener for external subagents
 */
#ifdef _WIN32

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
	// The ACE will allow either Everyone or given user to access pipe
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = (FILE_GENERIC_READ | FILE_GENERIC_WRITE) & ~FILE_CREATE_PIPE_INSTANCE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	const TCHAR *user = subagent->getUserName();
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
	}

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
	HANDLE hPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 8192, 8192, 0, &sa);
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

#else

static THREAD_RESULT THREAD_CALL ExternalSubagentConnector(void *arg)
{
	ExternalSubagent *subagent = (ExternalSubagent *)arg;
	mode_t prevMask = 0;

	int s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: socket failed (%s)"), _tcserror(errno));
		goto cleanup;
	}
	
	struct sockaddr_un addrLocal;
	addrLocal.sun_family = AF_UNIX;
#ifdef UNICODE
   sprintf(addrLocal.sun_path, "/tmp/.nxagentd.subagent.%S", subagent->getName());
#else
	sprintf(addrLocal.sun_path, "/tmp/.nxagentd.subagent.%s", subagent->getName());
#endif
	unlink(addrLocal.sun_path);
	prevMask = umask(S_IWGRP | S_IWOTH);
	if (bind(s, (struct sockaddr *)&addrLocal, SUN_LEN(&addrLocal)) == -1)
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: bind failed (%s)"), _tcserror(errno));
		umask(prevMask);
		goto cleanup;
	}
	umask(prevMask);

	if (listen(s, 5) == -1)
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: listen failed (%s)"), _tcserror(errno));
		goto cleanup;
	}
	
	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): UNIX socket created, waiting for connection"), subagent->getName());
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
		struct sockaddr_un addrRemote;
		socklen_t size = sizeof(struct sockaddr_un);
		SOCKET cs = accept(s, (struct sockaddr *)&addrRemote, &size);
		if (cs > 0)
		{
			subagent->connect(cs);
			shutdown(cs, 2);
			close(cs);
		}
		else
		{
			AgentWriteDebugLog(2, _T("ExternalSubagentConnector: accept failed (%s)"), _tcserror(errno));
		}
	}

cleanup:
	if (s != -1)
	{
		close(s);
	}

	AgentWriteDebugLog(2, _T("ExternalSubagentConnector: listener thread stopped"));
	return THREAD_OK;
}

#endif

/*
 * Send message to external subagent
 */
bool SendMessageToPipe(HPIPE hPipe, NXCP_MESSAGE *msg)
{
	bool success = false;

#ifdef _WIN32
   if (hPipe == INVALID_HANDLE_VALUE)
      return false;

	DWORD bytes = 0;
   if (WriteFile(hPipe, msg, ntohl(msg->size), &bytes, NULL))
	{
		success = (bytes == ntohl(msg->size));
	}
#else
   if (hPipe == -1)
      return false;

	int bytes = SendEx(hPipe, msg, ntohl(msg->size), 0, NULL); 
	success = (bytes == ntohl(msg->size));
#endif
	return success;
}

/**
 * Add external subagent from config.
 * Each line in config should be in form 
 * name:user
 * If user part is omited or set to *, connection from any account will be accepted
 */
bool AddExternalSubagent(const TCHAR *config)
{
	TCHAR buffer[1024], user[256] = _T("*");

	nx_strncpy(buffer, config, 1024);
	TCHAR *ptr = _tcschr(buffer, _T(':'));
	if (ptr != NULL)
	{
		*ptr = 0;
		ptr++;
		_tcsncpy(user, ptr, 256);
		Trim(user);
	}

	ExternalSubagent *subagent = new ExternalSubagent(buffer, user);
	s_subagents.add(subagent);
	ThreadCreate(ExternalSubagentConnector, 0, subagent);
	return true;
}

/**
 * Add parameters from external providers to NXCP message
 */
void ListParametersFromExtSubagents(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listParameters(msg, baseId, count);
		}
	}
}

/**
 * Add parameters from external subagents to string list
 */
void ListParametersFromExtSubagents(StringList *list)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listParameters(list);
		}
	}
}

/**
 * Add lists from external providers to NXCP message
 */
void ListListsFromExtSubagents(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listLists(msg, baseId, count);
		}
	}
}

/**
 * Add lists from external subagents to string list
 */
void ListListsFromExtSubagents(StringList *list)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listLists(list);
		}
	}
}

/**
 * Add tables from external providers to NXCP message
 */
void ListTablesFromExtSubagents(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listTables(msg, baseId, count);
		}
	}
}

/**
 * Add tables from external subagents to string list
 */
void ListTablesFromExtSubagents(StringList *list)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listTables(list);
		}
	}
}

/**
 * Get value from external subagent
 *
 * @return agent error code
 */
UINT32 GetParameterValueFromExtSubagent(const TCHAR *name, TCHAR *buffer)
{
	UINT32 rc = ERR_UNKNOWN_PARAMETER;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->getParameter(name, buffer);
			if (rc == ERR_SUCCESS)
				break;
		}
	}
	return rc;
}

/**
 * Get table from external subagent
 *
 * @return agent error code
 */
UINT32 GetTableValueFromExtSubagent(const TCHAR *name, Table *value)
{
	UINT32 rc = ERR_UNKNOWN_PARAMETER;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->getTable(name, value);
			if (rc == ERR_SUCCESS)
				break;
		}
	}
	return rc;
}

/**
 * Get list from external subagent
 *
 * @return agent error code
 */
UINT32 GetListValueFromExtSubagent(const TCHAR *name, StringList *value)
{
	UINT32 rc = ERR_UNKNOWN_PARAMETER;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->getList(name, value);
			if (rc == ERR_SUCCESS)
				break;
		}
	}
	return rc;
}

/**
 * Shutdown all connected external subagents
 */
void ShutdownExtSubagents()
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
         DebugPrintf(INVALID_INDEX, 1, _T("Sending SHUTDOWN command to external subagent %s"), s_subagents.get(i)->getName());
         s_subagents.get(i)->shutdown();
		}
	}
}
