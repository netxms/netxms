/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2024 Victor Kirhenshtein
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

#define DEBUG_TAG _T("extagent")

/**
 * Action list
 */
struct ActionList
{
   size_t count;
   ACTION *actions;
   TCHAR **strings;

   ActionList(uint32_t _count)
   {
      count = _count;
      actions = MemAllocArray<ACTION>(_count);
      strings = MemAllocArray<TCHAR*>(_count);
   }
   ~ActionList()
   {
      for(size_t i = 0; i < count; i++)
         MemFree(strings[i]);
      MemFree(strings);
      MemFree(actions);
   }
};

/**
 * List of registered external subagents
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
   m_listener = nullptr;
   m_pipe = nullptr;
   m_msgQueue = new MsgWaitQueue();
   m_requestId = 1;
   m_listenerStartDelay = 10000;
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
            (m_user[0] != 0) && _tcscmp(m_user, _T("*")) ? m_user : nullptr);
   if (m_listener != nullptr)
   {
      m_listener->start();
   }
   else
   {
      ThreadPoolScheduleRelative(g_commThreadPool, m_listenerStartDelay, this, &ExternalSubagent::startListener);
      if (m_listenerStartDelay < 300000)
         m_listenerStartDelay += m_listenerStartDelay / 2;
   }
}

/**
 * Stop listener
 */
void ExternalSubagent::stopListener()
{
   if (m_listener != nullptr)
      m_listener->stop();
}

/*
 * Send message to external subagent
 */
bool ExternalSubagent::sendMessage(const NXCPMessage *msg)
{
	TCHAR buffer[256];
	nxlog_debug_tag(DEBUG_TAG, 6, _T("ExternalSubagent::sendMessage(%s): sending message %s"), m_name, NXCPMessageCodeName(msg->getCode(), buffer));

	NXCP_MESSAGE *rawMsg = msg->serialize();
	bool success = (m_pipe != nullptr) ? m_pipe->write(rawMsg, ntohl(rawMsg->size)) : false;
	MemFree(rawMsg);
	return success;
}

/**
 * Wait for specific message to arrive
 */
NXCPMessage *ExternalSubagent::waitForMessage(uint16_t code, uint32_t id)
{
   return m_msgQueue->waitForMessage(code, id, 5000);	// 5 sec timeout
}

/**
 * Forward session message
 */
static void ForwardSessionMessage(NXCPMessage *msg)
{
   shared_ptr<AbstractCommSession> session = FindServerSessionById(msg->getId());
   if (session != nullptr)
      session->postRawMessage(reinterpret_cast<const NXCP_MESSAGE*>(msg->getBinaryData()));
}

/**
 * Main connection thread
 */
void ExternalSubagent::connect(NamedPipe *pipe)
{
   m_pipe = pipe;
   m_connected = true;
   nxlog_debug_tag(DEBUG_TAG, 2, _T("ExternalSubagent(%s): connection established"), m_name);

   syncPolicies();

   PipeMessageReceiver receiver(pipe->handle(), 8192, 1048576);  // 8K initial, 1M max
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(5000, &result);
		if (msg == nullptr)
      {
		   if (result == MSGRECV_TIMEOUT)
		      continue;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("ExternalSubagent(%s): receiver failure (%s)"), m_name, AbstractMessageReceiver::resultToText(result));
			break;
      }

	   TCHAR buffer[256];
		nxlog_debug_tag(DEBUG_TAG, 6, _T("ExternalSubagent(%s): received message %s"), m_name, NXCPMessageCodeName(msg->getCode(), buffer));
      switch(msg->getCode())
      {
         case CMD_PUSH_DCI_DATA:
            g_sessionLock.lock();
            for(int i = 0; i < g_sessions.size(); i++)
               if (g_sessions.get(i)->canAcceptTraps())
               {
                  g_sessions.get(i)->sendMessage(msg);
               }
            g_sessionLock.unlock();
            delete msg;
            break;
         case CMD_TRAP:
            ForwardEvent(msg);
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

	nxlog_debug_tag(DEBUG_TAG, 2, _T("ExternalSubagent(%s): connection closed"), m_name);
	m_connected = false;
	m_msgQueue->clear();
	m_pipe = nullptr;
}

/**
 * Send shutdown command with optional delayed restart
 */
void ExternalSubagent::shutdown(bool restart)
{
	NXCPMessage msg(CMD_SHUTDOWN, m_requestId++);
   msg.setField(VID_RESTART, restart);
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
	NETXMS_SUBAGENT_PARAM *result = nullptr;

	msg.setCode(CMD_GET_PARAMETER_LIST);
	msg.setId(m_requestId++);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != nullptr)
		{
			if (response->getFieldAsUInt32(VID_RCC) == ERR_SUCCESS)
			{
				*count = response->getFieldAsUInt32(VID_NUM_PARAMETERS);
				result = (NETXMS_SUBAGENT_PARAM *)malloc(*count * sizeof(NETXMS_SUBAGENT_PARAM));
				uint32_t fieldId = VID_PARAM_LIST_BASE;
				for(uint32_t i = 0; i < *count; i++)
				{
					response->getFieldAsString(fieldId++, result[i].name, MAX_PARAM_NAME);
					response->getFieldAsString(fieldId++, result[i].description, MAX_DB_STRING);
					result[i].dataType = response->getFieldAsInt16(fieldId++);
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
	NETXMS_SUBAGENT_LIST *result = nullptr;

	msg.setCode(CMD_GET_ENUM_LIST);
	msg.setId(m_requestId++);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != nullptr)
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
	NETXMS_SUBAGENT_TABLE *result = nullptr;

	msg.setCode(CMD_GET_TABLE_LIST);
	msg.setId(m_requestId++);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != nullptr)
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
ActionList *ExternalSubagent::getSupportedActions()
{
	NXCPMessage msg;
	ActionList *result = nullptr;

	msg.setCode(CMD_GET_ACTION_LIST);
	msg.setId(m_requestId++);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != nullptr)
		{
			if (response->getFieldAsUInt32(VID_RCC) == ERR_SUCCESS)
			{
				uint32_t count = response->getFieldAsUInt32(VID_NUM_ACTIONS);
				result = new ActionList(count);
				uint32_t varId = VID_ACTION_LIST_BASE;
				for(uint32_t i = 0; i < count; i++)
				{
               response->getFieldAsString(varId++, result->actions[i].name, MAX_PARAM_NAME);
               response->getFieldAsString(varId++, result->actions[i].description, MAX_DB_STRING);
               result->actions[i].isExternal = response->getFieldAsBoolean(varId++);
               result->strings[i] = response->getFieldAsString(varId++);
               if (result->actions[i].isExternal)
               {
                  result->actions[i].handler.cmdLine = result->strings[i];
               }
               else
               {
                  result->actions[i].handler.sa.subagentName = result->strings[i];
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
	if (list != nullptr)
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
		MemFree(list);
	}
}

/**
 * List supported parameters
 */
void ExternalSubagent::listParameters(StringList *list)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_PARAM *plist = getSupportedParameters(&paramCount);
	if (plist != nullptr)
	{
		for(UINT32 i = 0; i < paramCount; i++)
			list->add(plist[i].name);
		MemFree(plist);
	}
}

/**
 * List supported lists
 */
void ExternalSubagent::listLists(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_LIST *list = getSupportedLists(&paramCount);
	if (list != nullptr)
	{
		UINT32 id = *baseId;

		for(UINT32 i = 0; i < paramCount; i++)
		{
			msg->setField(id++, list[i].name);
		}
		*baseId = id;
		*count += paramCount;
		MemFree(list);
	}
}

/**
 * List supported lists
 */
void ExternalSubagent::listLists(StringList *list)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_LIST *plist = getSupportedLists(&paramCount);
	if (plist != nullptr)
	{
		for(UINT32 i = 0; i < paramCount; i++)
			list->add(plist[i].name);
		MemFree(plist);
	}
}

/**
 * List supported tables
 */
void ExternalSubagent::listTables(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	UINT32 paramCount = 0;
	NETXMS_SUBAGENT_TABLE *list = getSupportedTables(&paramCount);
	if (list != nullptr)
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
		MemFree(list);
	}
}

/**
 * List supported tables
 */
void ExternalSubagent::listTables(StringList *list)
{
	uint32_t paramCount = 0;
	NETXMS_SUBAGENT_TABLE *plist = getSupportedTables(&paramCount);
	if (plist != nullptr)
	{
		for(uint32_t i = 0; i < paramCount; i++)
			list->add(plist[i].name);
		MemFree(plist);
	}
}

/**
 * List supported actions
 */
void ExternalSubagent::listActions(NXCPMessage *msg, UINT32 *baseId, UINT32 *count)
{
	ActionList *list = getSupportedActions();
	if (list != nullptr)
	{
		uint32_t id = *baseId;
		for(size_t i = 0; i < list->count; i++)
		{
		   ACTION *a = &list->actions[i];
         msg->setField(id++, a->name);
         msg->setField(id++, a->description);
         msg->setField(id++, a->isExternal);
         msg->setField(id++, a->isExternal ? a->handler.cmdLine : a->handler.sa.subagentName);
		}
		*baseId = id;
		*count += static_cast<uint32_t>(list->count);
		delete list;
	}
}

/**
 * List supported actions
 */
void ExternalSubagent::listActions(StringList *list)
{
	ActionList *actions = getSupportedActions();
	if (actions != nullptr)
	{
      TCHAR buffer[1024];
		for(size_t i = 0; i < actions->count; i++)
      {
		   ACTION *a = &actions->actions[i];
         _sntprintf(buffer, 1024, _T("%s %s \"%s\""), a->name, a->isExternal ? _T("external") : _T("internal"),
                  a->isExternal ? a->handler.cmdLine : a->handler.sa.subagentName);
			list->add(buffer);
      }
		delete actions;
	}
}

/**
 * Get parameter value from external subagent
 */
uint32_t ExternalSubagent::getParameter(const TCHAR *name, TCHAR *buffer)
{
	NXCPMessage msg;
   uint32_t rcc;

	msg.setCode(CMD_GET_PARAMETER);
	msg.setId(m_requestId++);
	msg.setField(VID_PARAMETER, name);
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != nullptr)
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
uint32_t ExternalSubagent::getTable(const TCHAR *name, Table *value)
{
	NXCPMessage msg(CMD_GET_TABLE, m_requestId++);
	msg.setField(VID_PARAMETER, name);
   uint32_t rcc;
   if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != nullptr)
		{
			rcc = response->getFieldAsUInt32(VID_RCC);
			if (rcc == ERR_SUCCESS)
				value->updateFromMessage(*response);
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
uint32_t ExternalSubagent::getList(const TCHAR *name, StringList *value)
{
	NXCPMessage msg(CMD_GET_LIST, m_requestId++);
	msg.setField(VID_PARAMETER, name);
   uint32_t rcc;
   if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != nullptr)
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
uint32_t ExternalSubagent::executeAction(const TCHAR *name, const StringList& args, AbstractCommSession *session, uint32_t requestId, bool sendOutput)
{
   NXCPMessage msg(CMD_EXECUTE_ACTION, m_requestId++);
   msg.setField(VID_ACTION_NAME, name);
   args.fillMessage(&msg, VID_ACTION_ARG_BASE, VID_NUM_ARGS);
   msg.setField(VID_REQUEST_ID, requestId);
   msg.setField(VID_RECEIVE_OUTPUT, sendOutput);
   session->prepareProxySessionSetupMsg(&msg);

   uint32_t rcc;
	if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
		if (response != nullptr)
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
 * Synchronize agent policies
 */
void ExternalSubagent::syncPolicies()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT guid,type,server_info,server_id,version,content_hash FROM agent_policy"));
   if (hResult == nullptr)
      return;

   NXCPMessage msg(CMD_SYNC_AGENT_POLICIES, m_requestId++);
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   uint32_t count = 0;
   int rowCount = DBGetNumRows(hResult);
   for(int row = 0; row < rowCount; row++)
   {
      uuid guid = DBGetFieldGUID(hResult, row, 0);
      TCHAR type[32];
      DBGetField(hResult, row, 1, type, 32);

      TCHAR filePath[MAX_PATH], name[64];
      if (!_tcscmp(type, _T("AgentConfig")))
      {
         _sntprintf(filePath, MAX_PATH, _T("%s%s.xml"), g_szConfigPolicyDir, guid.toString(name));
      }
      else if (!_tcscmp(type, _T("LogParserConfig")))
      {
         _sntprintf(filePath, MAX_PATH, _T("%s%s.xml"), g_szLogParserDirectory, guid.toString(name));
      }
      else
      {
         // Ignore all other policy types
         continue;
      }

      size_t size;
      BYTE *content = LoadFile(filePath, &size);
      if (content == nullptr)
         continue;

      msg.setField(fieldId++, type);
      msg.setField(fieldId++, guid);
      msg.setField(fieldId++, content, size);
      TCHAR serverInfo[256];
      msg.setField(fieldId++, DBGetField(hResult, row, 2, serverInfo, 256));
      msg.setField(fieldId++, DBGetFieldUInt64(hResult, row, 3));
      msg.setField(fieldId++, DBGetFieldULong(hResult, row, 4));

      TCHAR hashAsText[33];
      if (DBGetField(hResult, row, 5, hashAsText, 33) != nullptr)
      {
         BYTE hash[MD5_DIGEST_SIZE];
         StrToBin(hashAsText, hash, MD5_DIGEST_SIZE);
         msg.setField(fieldId++, hash, MD5_DIGEST_SIZE);
      }
      else
      {
         fieldId++;
      }

      MemFree(content);
      count++;
      fieldId += 93;
   }
   DBFreeResult(hResult);

   msg.setField(VID_NUM_ELEMENTS, count);
   msg.setField(VID_DATA_DIRECTORY, g_szDataDirectory);
   sendMessage(&msg);
}

/**
 * Notify subagent that new policy is installed
 */
void ExternalSubagent::notifyOnPolicyInstall(const uuid& guid)
{
   NXCPMessage msg(CMD_DEPLOY_AGENT_POLICY, m_requestId++);
   msg.setField(VID_GUID, guid);
   sendMessage(&msg);
}

/**
 * Notify subagent that new component activation token is available
 */
void ExternalSubagent::notifyOnComponentToken(const AgentComponentToken *token)
{
   NXCPMessage msg(CMD_SET_COMPONENT_TOKEN, m_requestId++);
   msg.setField(VID_TOKEN, reinterpret_cast<const BYTE*>(token), sizeof(AgentComponentToken));
   sendMessage(&msg);
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

	_tcslcpy(buffer, config, 1024);
	TCHAR *ptr = _tcschr(buffer, _T(':'));
	if (ptr != nullptr)
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
uint32_t GetParameterValueFromExtSubagent(const TCHAR *name, TCHAR *buffer)
{
   uint32_t rc = ERR_UNKNOWN_METRIC;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->getParameter(name, buffer);
			if (rc != ERR_UNKNOWN_METRIC)
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
uint32_t GetTableValueFromExtSubagent(const TCHAR *name, Table *value)
{
   uint32_t rc = ERR_UNKNOWN_METRIC;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->getTable(name, value);
			if (rc != ERR_UNKNOWN_METRIC)
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
uint32_t GetListValueFromExtSubagent(const TCHAR *name, StringList *value)
{
   uint32_t rc = ERR_UNKNOWN_METRIC;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->getList(name, value);
			if (rc != ERR_UNKNOWN_METRIC)
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
uint32_t ExecuteActionByExtSubagent(const TCHAR *name, const StringList& args, const shared_ptr<AbstractCommSession>& session, uint32_t requestId, bool sendOutput)
{
   uint32_t rc = ERR_UNKNOWN_METRIC;
   for(int i = 0; i < s_subagents.size(); i++)
   {
      if (s_subagents.get(i)->isConnected())
      {
         rc = s_subagents.get(i)->executeAction(name, args, session.get(), requestId, sendOutput);
         if (rc != ERR_UNKNOWN_METRIC)
            break;
      }
   }
   return rc;
}

/**
 * Shutdown all connected external subagents
 */
void ShutdownExtSubagents(bool restart)
{
   for(int i = 0; i < s_subagents.size(); i++)
   {
      if (s_subagents.get(i)->isConnected())
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Sending SHUTDOWN command to external subagent %s"), s_subagents.get(i)->getName());
         s_subagents.get(i)->shutdown(restart);
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
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Sending RESTART command to external subagent %s"), s_subagents.get(i)->getName());
         s_subagents.get(i)->restart();
      }
   }
}

/**
 * Synchronize policies for all connected external subagents
 */
void SyncPoliciesWithExtSubagents()
{
   for(int i = 0; i < s_subagents.size(); i++)
   {
      if (s_subagents.get(i)->isConnected())
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Requesting policies synchronization with external subagent %s"), s_subagents.get(i)->getName());
         s_subagents.get(i)->syncPolicies();
      }
   }
}

/**
 * Notify all connected external subagents about policy change
 */
void NotifyExtSubagentsOnPolicyInstall(uuid *guid)
{
   for(int i = 0; i < s_subagents.size(); i++)
   {
      if (s_subagents.get(i)->isConnected())
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Sending policy installation notification to external subagent %s"), s_subagents.get(i)->getName());
         s_subagents.get(i)->notifyOnPolicyInstall(*guid);
      }
   }
   delete guid;
}

/**
 * Notify all connected external subagents about new componnet activation token
 */
void NotifyExtSubagentsOnComponentToken(const AgentComponentToken *token)
{
   for(int i = 0; i < s_subagents.size(); i++)
   {
      if (s_subagents.get(i)->isConnected())
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Sending component activation token notification to external subagent %s"), s_subagents.get(i)->getName());
         s_subagents.get(i)->notifyOnComponentToken(token);
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

/**
 * Get list of disconnected external subagents
 */
StringList GetDisconnectedExtSubagents()
{
   StringList ds;
   for (int i = 0; i < s_subagents.size(); i++)
   {
      if (!s_subagents.get(i)->isConnected())
         ds.add(s_subagents.get(i)->getName());
   }
   return ds;
}
