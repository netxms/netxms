/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: actions.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("action")

/**
 * Create new action
 */
Action::Action(const TCHAR *name)
{
   id = CreateUniqueId(IDG_ACTION);
   guid = uuid::generate();
   _tcsncpy(this->name, name, MAX_OBJECT_NAME);
   isDisabled = true;
   type = ServerActionType::LOCAL_COMMAND;
   emailSubject[0] = 0;
   rcptAddr[0] = 0;
   data = nullptr;
   channelName[0] = 0;
}

/**
 * Action constructor
 */
Action::Action(DB_RESULT hResult, int row)
{
   id = DBGetFieldULong(hResult, row, 0);
   guid = DBGetFieldGUID(hResult, row, 1);
   DBGetField(hResult, row, 2, name, MAX_OBJECT_NAME);
   type = static_cast<ServerActionType>(DBGetFieldLong(hResult, row, 3));
   isDisabled = DBGetFieldLong(hResult, row, 4) ? true : false;
   DBGetField(hResult, row, 5, rcptAddr, MAX_RCPT_ADDR_LEN);
   DBGetField(hResult, row, 6, emailSubject, MAX_EMAIL_SUBJECT_LEN);
   data = DBGetField(hResult, row, 7, NULL, 0);
   DBGetField(hResult, row, 8, channelName, MAX_OBJECT_NAME);
}

/**
 * Action copy constructor
 */
Action::Action(const Action& src)
{
   id = src.id;
   guid = src.guid;
   _tcscpy(name, src.name);
   type = src.type;
   isDisabled = src.isDisabled;
   _tcscpy(rcptAddr, src.rcptAddr);
   _tcscpy(emailSubject, src.emailSubject);
   data = MemCopyString(src.data);
   _tcscpy(channelName, src.channelName);
}

/**
 * Action destructor
 */
Action::~Action()
{
   MemFree(data);
}

/**
 * Fill NXCP message with action's data
 */
void Action::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_ACTION_ID, id);
   msg->setField(VID_GUID, guid);
   msg->setField(VID_IS_DISABLED, isDisabled);
   msg->setField(VID_ACTION_TYPE, static_cast<uint16_t>(type));
   msg->setField(VID_ACTION_DATA, CHECK_NULL_EX(data));
   msg->setField(VID_EMAIL_SUBJECT, emailSubject);
   msg->setField(VID_ACTION_NAME, name);
   msg->setField(VID_RCPT_ADDR, rcptAddr);
   msg->setField(VID_CHANNEL_NAME, channelName);
}

/**
 * Save action record to database
 */
void Action::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const TCHAR *columns[] = { _T("guid"), _T("action_name"), _T("action_type"),
            _T("is_disabled"), _T("rcpt_addr"), _T("email_subject"), _T("action_data"),
            _T("channel_name"), nullptr };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("actions"), _T("action_id"), id, columns);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(type));
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (INT32)(isDisabled ? 1 : 0));
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, rcptAddr, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, emailSubject, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, data, DB_BIND_STATIC);
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, channelName, DB_BIND_STATIC);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Static data
 */
static SynchronizedSharedHashMap<uint32_t, Action> s_actions;
static uint32_t s_updateCode;

/**
 * Action execution log record ID
 */
static VolatileCounter64 s_logRecordId = 0;

/**
 * Get last used actin execution log record ID
 */
int64_t GetLastActionExecutionLogId()
{
   return s_logRecordId;
}

/**
 * Send updates to all connected clients
 */
static void SendActionDBUpdate(ClientSession *session, const Action *action)
{
   session->onActionDBUpdate(s_updateCode, action);
}

/**
 * Load actions list from database
 */
bool LoadActions()
{
   bool success = false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT action_id,guid,action_name,action_type,is_disabled,rcpt_addr,email_subject,action_data,channel_name FROM actions ORDER BY action_id"));
   if (hResult != nullptr)
   {
      s_actions.clear();

      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         Action *action = new Action(hResult, i);
         s_actions.set(action->id, action);
      }

      nxlog_debug_tag(DEBUG_TAG, 2, _T("%d actions loaded"), s_actions.size());

      DBFreeResult(hResult);
      success = true;
   }
   else
   {
      nxlog_write(NXLOG_ERROR, _T("Error loading server action configuration from database"));
   }

   int64_t id = ConfigReadInt64(_T("LastActionExecutionLogRecordId"), 0);
   if (id > s_logRecordId)
      s_logRecordId = id;
   hResult = DBSelect(hdb, _T("SELECT max(id) FROM server_action_execution_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_logRecordId = std::max(DBGetFieldInt64(hResult, 0, 0), static_cast<int64_t>(s_logRecordId));
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Cleanup action-related stuff
 */
void CleanupActions()
{
   s_actions.clear();
}

/**
 * Execute remote action. Target can be given as IP address, host name,
 * object name in form @name, or object ID in form #id
 */
static bool ExecuteRemoteAction(const TCHAR *pszTarget, const TCHAR *pszAction)
{
   shared_ptr<AgentConnection> conn;
   if ((pszTarget[0] == '@') || (pszTarget[0] == '#'))
   {
      // Resolve object name or ID.
      shared_ptr<Node> node = (pszTarget[0] == '@') ?
            static_pointer_cast<Node>(FindObjectByName(&pszTarget[1], OBJECT_NODE)) :
            static_pointer_cast<Node>(FindObjectById(_tcstoul(&pszTarget[1], nullptr, 0), OBJECT_NODE));
      if (node == nullptr)
         return false;
      conn = node->getAgentConnection();
      if (conn == nullptr)
         return false;
   }
   else
   {
      // Resolve hostname
      InetAddress addr = InetAddress::resolveHostName(pszTarget);
      if (!addr.isValid())
         return false;

      shared_ptr<Node> node = FindNodeByIP(0, addr);
      if (node != nullptr)
      {
         conn = node->createAgentConnection();
         if (conn == nullptr)
            return false;
      }
      else
      {
         // Target node is not in our database, try default communication settings
         conn = make_shared<AgentConnection>(addr, AGENT_LISTEN_PORT, nullptr);
         conn->setCommandTimeout(g_agentCommandTimeout);
         if (!conn->connect(g_serverKey))
            return false;
      }
   }

   StringList *args = SplitCommandLine(pszAction);
   String command(args->get(0));
   args->remove(0);
   uint32_t rcc = conn->executeCommand(command, *args);
   delete args;

   return rcc == ERR_SUCCESS;
}

/**
 * Execute command on remote node by SSH. Node should have valid ssh proxy.
 */
static bool ExecuteRemoteSshAction(const TCHAR *target, const TCHAR *action)
{
   shared_ptr<Node> node;
   uint32_t proxyId = 0;
   if ((target[0] == '@') || (target[0] == '#'))
   {
      // Resolve object name or ID.
      node = (target[0] == '@') ?
            static_pointer_cast<Node>(FindObjectByName(&target[1], OBJECT_NODE)) :
            static_pointer_cast<Node>(FindObjectById(_tcstoul(&target[1], nullptr, 0), OBJECT_NODE));
   }
   else
   {
      // Resolve hostname
      InetAddress addr = InetAddress::resolveHostName(target);
      if (!addr.isValid())
         return false;

      node = FindNodeByIP(0, addr);
   }

   if (node != nullptr)
   {
      proxyId = node->getEffectiveSshProxy();
   }
   else
   {
      return false;
   }

   shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
   if (proxy != nullptr)
   {
      auto conn = proxy->getAgentConnection();
      if (conn != nullptr)
      {
         StringList list;
         TCHAR ipAddr[64];
         list.add(node->getIpAddress().toString(ipAddr));
         list.add(node->getSshPort());
         list.add(node->getSshLogin());
         list.add(node->getSshPassword());
         list.add(action);
         list.add(node->getSshKeyId());
         uint32_t rcc = conn->executeCommand(_T("SSH.Command"), list);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Agent action execution result: %d (%s)"), rcc, AgentErrorCodeToText(rcc));
         if (rcc == ERR_SUCCESS)
            return true;
      }
   }
   return false;
}

/**
 * Run external command via system()
 */
static void RunCommand(TCHAR *command)
{
   if (ConfigReadBoolean(_T("Server.EscapeLocalCommands"), false))
   {
      StringBuffer s(command);
#ifdef _WIN32
      s.replace(_T("\t"), _T("\\t"));
      s.replace(_T("\n"), _T("\\n"));
      s.replace(_T("\r"), _T("\\r"));
#else
      s.replace(_T("\t"), _T("\\\\t"));
      s.replace(_T("\n"), _T("\\\\n"));
      s.replace(_T("\r"), _T("\\\\r"));
#endif
      MemFree(command);
      command = MemCopyString(s);
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Executing command \"%s\""), command);
   if (_tsystem(command) == -1)
      nxlog_debug_tag(DEBUG_TAG, 5, _T("RunCommandThread: failed to execute command \"%s\""), command);
   MemFree(command);
}

/**
 * Forward event to other server
 */
static bool ForwardEvent(const TCHAR *server, const Event *event)
{
   InetAddress addr = InetAddress::resolveHostName(server);
	if (!addr.isValidUnicast())
	{
		nxlog_debug(2, _T("ForwardEvent: host name %s is invalid or cannot be resolved"), server);
		return false;
	}

	ISC *isc = new ISC(addr);
	UINT32 rcc = isc->connect(ISC_SERVICE_EVENT_FORWARDER);
	if (rcc == ISC_ERR_SUCCESS)
	{
		NXCPMessage msg;
		msg.setId(1);
		msg.setCode(CMD_FORWARD_EVENT);

		shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
		if (object != nullptr)
		{
         if (object->getObjectClass() == OBJECT_NODE)
         {
			   msg.setField(VID_IP_ADDRESS, static_cast<Node*>(object.get())->getIpAddress());
         }
			msg.setField(VID_EVENT_CODE, event->getCode());
			msg.setField(VID_EVENT_NAME, event->getName());
         msg.setField(VID_TAGS, event->getTagsAsList());
			msg.setField(VID_NUM_ARGS, (WORD)event->getParametersCount());
			for(int i = 0; i < event->getParametersCount(); i++)
				msg.setField(VID_EVENT_ARG_BASE + i, event->getParameter(i));

			const StringList *list = event->getParameterNames();
			if (!list->isEmpty())
			{
	         for(int i = 0; i < list->size(); i++)
	            msg.setField(VID_EVENT_ARG_NAMES_BASE + i, list->get(i));
			}

			if (isc->sendMessage(&msg))
			{
				rcc = isc->waitForRCC(1, 10000);
			}
			else
			{
				rcc = ISC_ERR_CONNECTION_BROKEN;
			}
		}
		else
		{
			rcc = ISC_ERR_INTERNAL_ERROR;
		}
		isc->disconnect();
	}
	delete isc;
	if (rcc != ISC_ERR_SUCCESS)
		nxlog_write(NXLOG_WARNING, _T("Failed to forward event to server %s (%s)"), server, ISCErrorCodeToText(rcc));
	return rcc == ISC_ERR_SUCCESS;
}

/**
 * Execute NXSL script
 */
static bool ExecuteActionScript(const TCHAR *script, const Event *event)
{
   TCHAR name[1024];
   _tcslcpy(name, script, 1024);
   Trim(name);

   // Can be in form parameter(arg1, arg2, ... argN)
   TCHAR *p = _tcschr(name, _T('('));
   if (p != nullptr)
   {
      size_t l = _tcslen(name) - 1;
      if (name[l] != _T(')'))
         return false;
      name[l] = 0;
      *p = 0;
   }

   bool success = false;
	ScriptVMHandle vm = CreateServerScriptVM(name, FindObjectById(event->getSourceId()));
	if (vm.isValid())
	{
		vm->setGlobalVariable("$event", vm->createValue(vm->createObject(&g_nxslEventClass, event, true)));

      ObjectRefArray<NXSL_Value> args(16, 16);
      if ((p == nullptr) || ParseValueList(vm, &p, args, true))
      {
         // Pass event's parameters as arguments
         for(int i = 0; i < event->getParametersCount(); i++)
            args.add(vm->createValue(event->getParameter(i)));

         if (vm->run(args))
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("ExecuteActionScript: script %s successfully executed"), name);
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("ExecuteActionScript: Script %s execution error: %s"), name, vm->getErrorText());
            ReportScriptError(SCRIPT_CONTEXT_ACTION, FindObjectById(event->getSourceId()).get(), 0, vm->getErrorText(), name);
         }
      }
      else
      {
         // argument parsing error
         nxlog_debug(6, _T("ExecuteActionScript: Argument parsing error for script %s"), name);
      }
      vm.destroy();
	}
	else
	{
	   ReportScriptError(_T("ExecuteActionScript"), nullptr, 0, vm.failureReasonText(), _T("ScriptLibrarrie::%s"), name);
	}
	return success;
}

/**
 * Saves server action execution log record to DB
 */
static void WriteServerActionExecutionLog(uint64_t eventId, uint32_t eventCode, uint32_t actionId, const TCHAR* actionName, const TCHAR* channelName, const TCHAR* recipient, const TCHAR* subject, const TCHAR* body, bool success)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
         g_dbSyntax == DB_SYNTAX_TSDB ?
            _T("INSERT INTO server_action_execution_log (id,action_timestamp,action_id,action_name,channel_name,recipient,subject,action_data,event_id,event_code,success) VALUES (?,to_timestamp(?),?,?,?,?,?,?,?,?,?)") :
            _T("INSERT INTO server_action_execution_log (id,action_timestamp,action_id,action_name,channel_name,recipient,subject,action_data,event_id,event_code,success) VALUES (?,?,?,?,?,?,?,?,?,?,?)"));

   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, InterlockedIncrement64(&s_logRecordId));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(time(nullptr)));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, actionId);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, actionName, DB_BIND_STATIC, 63);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, channelName, DB_BIND_STATIC, 63);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, recipient, DB_BIND_STATIC, 2000);
      DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, subject, DB_BIND_STATIC, 2000);
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, body, DB_BIND_STATIC, 2000);
      DBBind(hStmt, 9, DB_SQLTYPE_BIGINT, eventId);
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, eventCode);
      DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, success ? _T("1") : _T("0"), DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Execute action on specific event
 */
bool ExecuteAction(uint32_t actionId, const Event *event, const Alarm *alarm)
{
   static const TCHAR *actionType[] = { _T("EXEC"), _T("REMOTE"), _T("SEND EMAIL"), _T("SEND NOTIFICATION"), _T("FORWARD EVENT"), _T("NXSL SCRIPT"), _T("XMPP MESSAGE"), _T("SSH") };

   bool success = false;

   shared_ptr<Action> action = s_actions.getShared(actionId);
   if (action != nullptr)
   {
      if (action->isDisabled)
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Action %d (%s) is disabled and will not be executed"), actionId, action->name);
         success = true;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Executing action %d (%s) of type %s"), actionId, action->name, actionType[static_cast<int>(action->type)]);

         StringBuffer expandedData = event->expandText(CHECK_NULL_EX(action->data), alarm);
         expandedData.trim();

         StringBuffer expandedRcpt = event->expandText(action->rcptAddr, alarm);
         expandedRcpt.trim();

         String expandedSubject = event->expandText(action->emailSubject, alarm);

         switch(action->type)
         {
            case ServerActionType::LOCAL_COMMAND:
               if (!expandedData.isEmpty())
               {
                  ThreadPoolExecute(g_mainThreadPool, RunCommand, MemCopyString(expandedData));
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty command - nothing to execute"));
               }
               success = true;
               break;
            case ServerActionType::NOTIFICATION:
               if (action->channelName[0] != 0)
               {
                  if(expandedRcpt.isEmpty())
                     nxlog_debug_tag(DEBUG_TAG, 3, _T("Sending notification using channel %s: \"%s\""), action->channelName, (const TCHAR *)expandedData);

                  else
                     nxlog_debug_tag(DEBUG_TAG, 3, _T("Sending notification using channel %s to %s: \"%s\""), action->channelName, (const TCHAR *)expandedRcpt, (const TCHAR *)expandedData);
                  SendNotification(action->channelName, expandedRcpt.getBuffer(), expandedSubject, expandedData);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty channel name - notification will not be sent"));
               }
               success = true;
               break;
            case ServerActionType::AGENT_COMMAND:
               if (!expandedRcpt.isEmpty())
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Executing on \"%s\": \"%s\""), expandedRcpt.cstr(), expandedData.cstr());
                  success = ExecuteRemoteAction(expandedRcpt, expandedData);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty target list - remote action will not be executed"));
                  success = true;
               }
               break;
            case ServerActionType::FORWARD_EVENT:
               if (!expandedRcpt.isEmpty())
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Forwarding event to \"%s\""), expandedRcpt.cstr());
                  success = ForwardEvent(expandedRcpt, event);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty destination - event will not be forwarded"));
                  success = true;
               }
               break;
            case ServerActionType::NXSL_SCRIPT:
               if (!expandedRcpt.isEmpty())
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Executing NXSL script \"%s\""), expandedRcpt.cstr());
                  success = ExecuteActionScript(expandedRcpt, event);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty script name - nothing to execute"));
                  success = true;
               }
               break;
            case ServerActionType::SSH_COMMAND:
               if (!expandedRcpt.isEmpty())
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Executing by ssh on \"%s\": \"%s\""), expandedRcpt.cstr(), expandedData.cstr());
                  success = ExecuteRemoteSshAction(expandedRcpt, expandedData);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty target list - remote ssh action will not be executed"));
                  success = true;
               }
               break;
            default:
               break;
         }
         WriteServerActionExecutionLog(event->getId(), event->getCode(), action->id, action->name, action->channelName, expandedRcpt, expandedSubject, expandedData, success);
      }
   }
   return success;
}

/**
 * Action name comparator
 */
static bool ActionNameComparator(const uint32_t& id, const Action& action, const TCHAR *name)
{
   return _tcsicmp(action.name, name) == 0;
}

/**
 * Create new action
 */
uint32_t CreateAction(const TCHAR *name, uint32_t *id)
{
   // Check for duplicate name
   if (s_actions.findElement(ActionNameComparator, name) != nullptr)
      return RCC_OBJECT_ALREADY_EXISTS;

   Action *action = new Action(name);
   *id = action->id;
   action->saveToDatabase();

   s_actions.set(action->id, action);
   s_updateCode = NX_NOTIFY_ACTION_CREATED;
   EnumerateClientSessions(SendActionDBUpdate, action);

   return RCC_SUCCESS;
}

/**
 * Delete action
 */
uint32_t DeleteAction(uint32_t actionId)
{
   uint32_t dwResult = RCC_SUCCESS;

   shared_ptr<Action> action = s_actions.getShared(actionId);
   if (action != nullptr)
   {
      s_updateCode = NX_NOTIFY_ACTION_DELETED;
      EnumerateClientSessions(SendActionDBUpdate, action.get());
      s_actions.remove(actionId);
   }
   else
   {
      dwResult = RCC_INVALID_ACTION_ID;
   }

   if (dwResult == RCC_SUCCESS)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      ExecuteQueryOnObject(hdb, actionId, _T("DELETE FROM actions WHERE action_id=?"));
      DBConnectionPoolReleaseConnection(hdb);
   }
   return dwResult;
}

/**
 * Modify action record from message
 */
uint32_t ModifyActionFromMessage(const NXCPMessage& msg)
{
   uint32_t rcc = RCC_INVALID_ACTION_ID;

   TCHAR name[MAX_OBJECT_NAME];
   msg.getFieldAsString(VID_ACTION_NAME, name, MAX_OBJECT_NAME);
   uint32_t actionId = msg.getFieldAsUInt32(VID_ACTION_ID);

   shared_ptr<Action> tmp = s_actions.getShared(actionId);
   if (tmp != nullptr)
   {
      auto action = make_shared<Action>(*tmp);
      action->isDisabled = msg.getFieldAsBoolean(VID_IS_DISABLED);
      action->type = static_cast<ServerActionType>(msg.getFieldAsUInt16(VID_ACTION_TYPE));
      MemFree(action->data);
      action->data = msg.getFieldAsString(VID_ACTION_DATA);
      msg.getFieldAsString(VID_EMAIL_SUBJECT, action->emailSubject, MAX_EMAIL_SUBJECT_LEN);
      msg.getFieldAsString(VID_RCPT_ADDR, action->rcptAddr, MAX_RCPT_ADDR_LEN);
      msg.getFieldAsString(VID_CHANNEL_NAME, action->channelName, MAX_OBJECT_NAME);
      _tcscpy(action->name, name);

      action->saveToDatabase();

      s_updateCode = NX_NOTIFY_ACTION_MODIFIED;
      EnumerateClientSessions(SendActionDBUpdate, action.get());
      s_actions.set(actionId, action);

      rcc = RCC_SUCCESS;
   }

   return rcc;
}

/**
 * Send action to client
 * first - old name
 * second - new name
 */
static EnumerationCallbackResult RenameChannel(const uint32_t& id, const shared_ptr<Action>& action, std::pair<TCHAR*, TCHAR*> *names)
{
   if (!_tcsncmp(action->channelName, names->first, MAX_OBJECT_NAME) && (action->type == ServerActionType::NOTIFICATION))
   {
      _tcslcpy(action->channelName, names->second, MAX_OBJECT_NAME);
      s_updateCode = NX_NOTIFY_ACTION_MODIFIED;
      EnumerateClientSessions(SendActionDBUpdate, action.get());
   }
   return _CONTINUE;
}

/**
 * Update notification channel name in all actions
 * first - old name
 * second - new name
 */
void UpdateChannelNameInActions(std::pair<TCHAR*, TCHAR*> *names)
{
   s_actions.forEach(RenameChannel, names);
}

/**
 * Check if notification channel with given name is used in actions
 */
static bool CheckChannelUsage(const uint32_t& id, const Action& action, TCHAR *name)
{
   return _tcsncmp(action.channelName, name, MAX_OBJECT_NAME) == 0;
}

/**
 * Check if notification channel with given name is used in actions
 */
bool CheckChannelIsUsedInAction(TCHAR *name)
{
   return s_actions.findElement(CheckChannelUsage, name) != nullptr;
}

/**
 * Sender callbact data
 */
struct SendActionData
{
   ClientSession *session;
   NXCPMessage *message;
};

/**
 * Send action to client
 */
static EnumerationCallbackResult SendAction(const uint32_t& id, const shared_ptr<Action>& action, SendActionData *data)
{
   action->fillMessage(data->message);
   data->session->sendMessage(data->message);
   data->message->deleteAllFields();
   return _CONTINUE;
}

/**
 * Send all actions to client
 */
void SendActionsToClient(ClientSession *session, uint32_t requestId)
{
   NXCPMessage msg(CMD_ACTION_DATA, requestId);

   SendActionData data;
   data.session = session;
   data.message = &msg;
   s_actions.forEach(SendAction, &data);

   // Send end-of-list flag
   msg.setField(VID_ACTION_ID, (UINT32)0);
   session->sendMessage(&msg);
}

/**
 * Export action configuration
 */
void CreateActionExportRecord(StringBuffer &xml, uint32_t id)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT guid,action_name,action_type,rcpt_addr,email_subject,action_data,channel_name FROM actions WHERE action_id=?"));
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         xml.append(_T("\t\t<action id=\""))
            .append(id)
            .append(_T("\">\n\t\t\t<guid>"))
            .append(DBGetFieldGUID(hResult, 0, 0))
            .append(_T("</guid>\n\t\t\t<name>"))
            .appendPreallocated(DBGetFieldForXML(hResult, 0, 1))
            .append(_T("</name>\n\t\t\t<type>"))
            .append(DBGetFieldULong(hResult, 0, 2))
            .append(_T("</type>\n\t\t\t<recipientAddress>"))
            .appendPreallocated(DBGetFieldForXML(hResult, 0, 3))
            .append(_T("</recipientAddress>\n\t\t\t<emailSubject>"))
            .appendPreallocated(DBGetFieldForXML(hResult, 0, 4))
            .append(_T("</emailSubject>\n\t\t\t<data>"))
            .appendPreallocated(DBGetFieldForXML(hResult, 0, 5))
            .append(_T("</data>\n\t\t\t<channelName>"))
            .appendPreallocated(DBGetFieldForXML(hResult, 0, 6))
            .append(_T("</channelName>\n"))
            .append(_T("\t\t</action>\n"));
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Check if given action ID is valid
 */
bool IsValidActionId(uint32_t id)
{
   return s_actions.getShared(id) != nullptr;
}

/**
 * Get GUID of given action
 */
uuid GetActionGUID(uint32_t id)
{
   shared_ptr<Action> action = s_actions.getShared(id);
   uuid guid = (action != NULL) ? action->guid : uuid::NULL_UUID;
   return guid;
}

/**
 * Action GUID comparator
 */
static bool ActionGUIDComparator(const uint32_t& id, const Action& action, const uuid *data)
{
   return action.guid.equals(*data);
}

/**
 * Find action by GUID
 */
uint32_t FindActionByGUID(const uuid& guid)
{
   shared_ptr<Action> action = s_actions.findElement(ActionGUIDComparator, &guid);
   uint32_t id = (action != nullptr) ? action->id : 0;
   return id;
}

/**
 * Import action configuration
 */
bool ImportAction(ConfigEntry *config, bool overwrite)
{
   if (config->getSubEntryValue(_T("name")) == nullptr)
   {
      nxlog_debug_tag(_T("import"), 4, _T("ImportAction: no name specified"));
      return false;
   }

   const uuid guid = config->getSubEntryValueAsUUID(_T("guid"));
   shared_ptr<Action> action = !guid.isNull() ? s_actions.findElement(ActionGUIDComparator, &guid) : shared_ptr<Action>();
   if (action == nullptr)
   {
      // Check for duplicate name
      const TCHAR *name = config->getSubEntryValue(_T("name"));
      if (s_actions.findElement(ActionNameComparator, name) != NULL)
      {
         nxlog_debug_tag(_T("import"), 4, _T("ImportAction: name \"%s\" already exists"), name);
         return false;
      }

      action = make_shared<Action>(name);
      action->isDisabled = false;
      if (!guid.isNull())
         action->guid = guid;
      s_updateCode = NX_NOTIFY_ACTION_CREATED;
   }
   else
   {
      TCHAR guidText[64];
      if (!overwrite)
      {
         nxlog_debug_tag(_T("import"), 4, _T("ImportAction: found existing action \"%s\" with GUID %s (skipping)"),
                  action->name, action->guid.toString(guidText));
         return true;
      }
      nxlog_debug_tag(_T("import"), 4, _T("ImportAction: found existing action \"%s\" with GUID %s"), action->name, action->guid.toString(guidText));
      _tcslcpy(action->name, config->getSubEntryValue(_T("name")), MAX_OBJECT_NAME);
      s_updateCode = NX_NOTIFY_ACTION_MODIFIED;
      action = make_shared<Action>(*action);
   }

   // If not exist, create it
   action->type = static_cast<ServerActionType>(config->getSubEntryValueAsInt(_T("type"), static_cast<int>(ServerActionType::LOCAL_COMMAND)));
   if (config->getSubEntryValue(_T("emailSubject")) == NULL)
      action->emailSubject[0] = 0;
   else
      _tcslcpy(action->emailSubject, config->getSubEntryValue(_T("emailSubject")), MAX_EMAIL_SUBJECT_LEN);
   if (config->getSubEntryValue(_T("recipientAddress")) == NULL)
      action->rcptAddr[0] = 0;
   else
      _tcslcpy(action->rcptAddr, config->getSubEntryValue(_T("recipientAddress")), MAX_RCPT_ADDR_LEN);
   if (config->getSubEntryValue(_T("channelName")) == NULL)
      action->channelName[0] = 0;
   else
      _tcslcpy(action->channelName, config->getSubEntryValue(_T("channelName")), MAX_OBJECT_NAME);
   action->data = MemCopyString(config->getSubEntryValue(_T("data")));
   action->saveToDatabase();
   EnumerateClientSessions(SendActionDBUpdate, action.get());
   s_actions.set(action->id, action);

   return true;
}

/**
 * Task handler for scheduled action execution
 */
void ExecuteScheduledAction(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   uint32_t actionId = ExtractNamedOptionValueAsUInt(parameters->m_persistentData, _T("action"), 0);
   Event *restoredEvent = nullptr;
   Alarm *restoredAlarm = nullptr;
   const Event *event;
   const Alarm *alarm;
   if (parameters->m_transientData != nullptr)
   {
      event = static_cast<ActionExecutionTransientData*>(parameters->m_transientData)->getEvent();
      alarm = static_cast<ActionExecutionTransientData*>(parameters->m_transientData)->getAlarm();
   }
   else
   {
      uint64_t eventId = ExtractNamedOptionValueAsUInt64(parameters->m_persistentData, _T("event"), 0);
      if (eventId != 0)
      {
         restoredEvent = LoadEventFromDatabase(eventId);
      }

      uint32_t alarmId = ExtractNamedOptionValueAsUInt(parameters->m_persistentData, _T("alarm"), 0);
      if (alarmId != 0)
      {
         restoredAlarm = FindAlarmById(alarmId);
         if (restoredAlarm == nullptr)
            restoredAlarm = LoadAlarmFromDatabase(alarmId);
      }

      event = restoredEvent;
      alarm = restoredAlarm;
   }

   if (event != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Executing scheduled action [%u] for event %s on node [%u]"),
               actionId, event->getName(), parameters->m_objectId);
      ExecuteAction(actionId, event, alarm);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot execute scheduled action [%u] on node [%u]: original event is unavailable"),
               actionId, parameters->m_objectId);
   }
   delete restoredEvent;
   delete restoredAlarm;
}

/**
 * Constructor for scheduled action execution transient data
 */
ActionExecutionTransientData::ActionExecutionTransientData(const Event *e, const Alarm *a)
{
   m_event = new Event(e);
   m_alarm = (a != nullptr) ? new Alarm(a, false) : nullptr;
}

/**
 * Destructor for scheduled action execution transient data
 */
ActionExecutionTransientData::~ActionExecutionTransientData()
{
   delete m_event;
   delete m_alarm;
}
