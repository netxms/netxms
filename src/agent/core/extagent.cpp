/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
	_tcslcpy(m_name, name, MAX_SUBAGENT_NAME);
	_tcslcpy(m_user, user, MAX_ESA_USER_NAME);
	m_connected = false;
	m_listener = NULL;
	m_pipe = NULL;
	m_msgQueue = new MsgWaitQueue();
	m_requestId = 1;
}

/**
 * Destructor
 */
ExternalSubagent::~ExternalSubagent()
{
	delete m_msgQueue;
	delete m_listener;
}

/**
 * Pipe request handler
 */
static void RequestHandler(NamedPipe *pipe, void *userArg)
{
   static_cast<ExternalSubagent*>(userArg)->connect(pipe);
}

/**
 * Start listener
 */
void ExternalSubagent::startListener()
{
   TCHAR name[MAX_PIPE_NAME_LEN];
   _sntprintf(name, MAX_PIPE_NAME_LEN, _T("nxagentd.subagent.%s"), m_name);
   m_listener = NamedPipeListener::create(name, RequestHandler, this,
            (m_user[0] != 0) && _tcscmp(m_user, _T("*")) ? m_user : NULL);
   if (m_listener != NULL)
      m_listener->start();
}

/**
 * Stop listener
 */
void ExternalSubagent::stopListener()
{
   m_listener->stop();
}

/*
 * Send message to external subagent
 */
bool ExternalSubagent::sendMessage(const NXCPMessage *msg)
{
	TCHAR buffer[256];
	AgentWriteDebugLog(6, _T("ExternalSubagent::sendMessage(%s): sending message %s"), m_name, NXCPMessageCodeName(msg->getCode(), buffer));

	NXCP_MESSAGE *rawMsg = msg->serialize();
	bool success = (m_pipe != NULL) ? m_pipe->write(rawMsg, ntohl(rawMsg->size)) : false;
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
 * Forward session message
 */
static void ForwardSessionMessage(NXCPMessage *msg)
{
   AbstractCommSession *session = FindServerSessionById(msg->getId());
   if (session != NULL)
   {
      session->postRawMessage(reinterpret_cast<const NXCP_MESSAGE*>(msg->getBinaryData()));
      session->decRefCount();
   }
}

/**
 * Main connection thread
 */
void ExternalSubagent::connect(NamedPipe *pipe)
{
	TCHAR buffer[256];
   UINT32 i;

	m_pipe = pipe;
	m_connected = true;
	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): connection established"), m_name);
   PipeMessageReceiver receiver(pipe->handle(), 8192, 1048576);  // 8K initial, 1M max
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(5000, &result);
		if (msg == NULL)
      {
		   if (result == MSGRECV_TIMEOUT)
		      continue;
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
         case CMD_PROXY_MESSAGE:
            ForwardSessionMessage(msg);
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
	m_pipe = NULL;
}

/**
 * Send shutdown command
 */
void ExternalSubagent::shutdown()
{
	NXCPMessage msg(CMD_SHUTDOWN, m_requestId++);
	sendMessage(&msg);
}

/**
 * Send restart command
 */
void ExternalSubagent::restart()
{
   NXCPMessage msg(CMD_RESTART, m_requestId++);
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
 * Get list of supported tables
 */
ACTION *ExternalSubagent::getSupportedActions(UINT32 *count)
{
	NXCPMessage msg;
	ACTION *result = NULL;

	msg.setCode(CMD_GET_ACTION_LIST);
	msg.setId(m_requestId++);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != NULL)
		{
			if (response->getFieldAsUInt32(VID_RCC) == ERR_SUCCESS)
			{
				*count = response->getFieldAsUInt32(VID_NUM_ACTIONS);
				result = (ACTION *)calloc(*count, sizeof(ACTION));
				UINT32 varId = VID_ACTION_LIST_BASE;
				for(UINT32 i = 0; i < *count; i++)
				{
               response->getFieldAsString(varId++, result[i].szName, MAX_PARAM_NAME);
               response->getFieldAsString(varId++, result[i].szDescription, MAX_DB_STRING);
               result[i].iType = response->getFieldAsInt16(varId++);
               if (result[i].iType == AGENT_ACTION_SUBAGENT)
               {
                  response->getFieldAsString(varId++, result[i].handler.sa.szSubagentName, MAX_PATH);
               }
               else
               {
                  result[i].handler.pszCmdLine = response->getFieldAsString(varId++);
               }
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
 * List supported actions
 */
void ExternalSubagent::listActions(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	UINT32 actionCount = 0;
	ACTION *list = getSupportedActions(&actionCount);
	if (list != NULL)
	{
		UINT32 id = *baseId;

		for(UINT32 i = 0; i < actionCount; i++)
		{
         msg->setField(id++, list[i].szName);
         msg->setField(id++, list[i].szDescription);
         msg->setField(id++, (INT16)list[i].iType);
         msg->setField(id++, (list[i].iType == AGENT_ACTION_SUBAGENT) ? list[i].handler.sa.szSubagentName : list[i].handler.pszCmdLine);
		}
		*baseId = id;
		*count += actionCount;
		free(list);
	}
}

/**
 * List supported actions
 */
void ExternalSubagent::listActions(StringList *list)
{
	UINT32 actionCount = 0;
	ACTION *plist = getSupportedActions(&actionCount);
	if (plist != NULL)
	{
      TCHAR buffer[1024];
		for(UINT32 i = 0; i < actionCount; i++)
      {
         _sntprintf(buffer, 1024, _T("%s %d \"%s\""), plist[i].szName, plist[i].iType,
                    plist[i].iType == AGENT_ACTION_SUBAGENT ?
                       plist[i].handler.sa.szSubagentName :    
                       plist[i].handler.pszCmdLine);
			list->add(buffer);
      }
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
 * Execute action by remote subagent
 */
UINT32 ExternalSubagent::executeAction(const TCHAR *name, const StringList *args, AbstractCommSession *session, UINT32 requestId, bool sendOutput)
{
	NXCPMessage msg(CMD_EXECUTE_ACTION, m_requestId++);
	msg.setField(VID_ACTION_NAME, name);
   args->fillMessage(&msg, VID_ACTION_ARG_BASE, VID_NUM_ARGS);
   msg.setField(VID_REQUEST_ID, requestId);
   msg.setField(VID_RECEIVE_OUTPUT, sendOutput);
   session->prepareProxySessionSetupMsg(&msg);

   UINT32 rcc;
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != NULL)
		{
			rcc = response->getFieldAsUInt32(VID_RCC);
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
	subagent->startListener();
	return true;
}

/**
 * Stop external subagent connectors
 */
void StopExternalSubagentConnectors()
{
   for(int i = 0; i < s_subagents.size(); i++)
      s_subagents.get(i)->stopListener();
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
 * Add actions from external providers to NXCP message
 */
void ListActionsFromExtSubagents(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listActions(msg, baseId, count);
		}
	}
}

/**
 * Add actions from external subagents to string list
 */
void ListActionsFromExtSubagents(StringList *list)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listActions(list);
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
			if (rc != ERR_UNKNOWN_PARAMETER)
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
			if (rc != ERR_UNKNOWN_PARAMETER)
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
			if (rc != ERR_UNKNOWN_PARAMETER)
				break;
		}
	}
	return rc;
}

/**
 * Execute action by external subagent
 *
 * @return agent error code
 */
UINT32 ExecuteActionByExtSubagent(const TCHAR *name, const StringList *args, AbstractCommSession *session, UINT32 requestId, bool sendOutput)
{
	UINT32 rc = ERR_UNKNOWN_PARAMETER;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->executeAction(name, args, session, requestId, sendOutput);
			if (rc != ERR_UNKNOWN_PARAMETER)
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
         nxlog_debug(1, _T("Sending SHUTDOWN command to external subagent %s"), s_subagents.get(i)->getName());
         s_subagents.get(i)->shutdown();
		}
	}
}

/**
 * Restart all connected external subagents
 */
void RestartExtSubagents()
{
   for(int i = 0; i < s_subagents.size(); i++)
   {
      if (s_subagents.get(i)->isConnected())
      {
         nxlog_debug(1, _T("Sending RESTART command to external subagent %s"), s_subagents.get(i)->getName());
         s_subagents.get(i)->restart();
      }
   }
}

/**
 * Handler for Agent.IsExternalSubagentConnected
 */
LONG H_IsExtSubagentConnected(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[256];
   if (!AgentGetParameterArg(cmd, 1, name, 256))
      return SYSINFO_RC_UNSUPPORTED;
   LONG rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   for(int i = 0; i < s_subagents.size(); i++)
   {
      if (!_tcsicmp(s_subagents.get(i)->getName(), name))
      {
         ret_int(value, s_subagents.get(i)->isConnected() ? 1 : 0);
         rc = SYSINFO_RC_SUCCESS;
         break;
      }
   }
   return rc;
}
