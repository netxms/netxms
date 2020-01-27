/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
   type = ACTION_EXEC;
   emailSubject[0] = 0;
   rcptAddr[0] = 0;
   data = NULL;
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
   type = DBGetFieldLong(hResult, row, 3);
   isDisabled = DBGetFieldLong(hResult, row, 4) ? true : false;
   DBGetField(hResult, row, 5, rcptAddr, MAX_RCPT_ADDR_LEN);
   DBGetField(hResult, row, 6, emailSubject, MAX_EMAIL_SUBJECT_LEN);
   data = DBGetField(hResult, row, 7, NULL, 0);
   DBGetField(hResult, row, 8, channelName, MAX_OBJECT_NAME);
}

/**
 * Action copy constructor
 */
Action::Action(const Action *act)
{
   id = act->id;
   guid = act->guid;
   _tcsncpy(name, act->name, MAX_OBJECT_NAME);
   type = act->type;
   isDisabled = act->isDisabled;
   _tcsncpy(rcptAddr, act->rcptAddr, MAX_RCPT_ADDR_LEN);
   _tcsncpy(emailSubject, act->emailSubject, MAX_EMAIL_SUBJECT_LEN);
   data = MemCopyString(act->data);
   _tcsncpy(channelName, act->channelName, MAX_OBJECT_NAME);

}

/**
 * Action destructor
 */
Action::~Action()
{
   free(data);
}

/**
 * Fill NXCP message with action's data
 */
void Action::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_ACTION_ID, id);
   msg->setField(VID_GUID, guid);
   msg->setField(VID_IS_DISABLED, isDisabled);
   msg->setField(VID_ACTION_TYPE, (UINT16)type);
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
            _T("channel_name"), NULL };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("actions"), _T("action_id"), id, columns);
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, type);
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
static SynchronizedSharedHashMap<UINT32, Action> s_actions(true);
static UINT32 m_dwUpdateCode;

/**
 * Send updates to all connected clients
 */
static void SendActionDBUpdate(ClientSession *pSession, void *pArg)
{
   pSession->onActionDBUpdate(m_dwUpdateCode, (const Action *)pArg);
}

/**
 * Load actions list from database
 */
bool LoadActions()
{
   bool success = false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT action_id,guid,action_name,action_type,")
                                     _T("is_disabled,rcpt_addr,email_subject,action_data,")
                                     _T("channel_name FROM actions ORDER BY action_id"));
   if (hResult != NULL)
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
 * Execute remote action
 */
static BOOL ExecuteRemoteAction(const TCHAR *pszTarget, const TCHAR *pszAction)
{
   AgentConnection *pConn;
   if (pszTarget[0] == '@')
   {
      //Resolve name of node to connection to it. Name should be in @name format.
      Node *node = (Node *)FindObjectByName(&pszTarget[1], OBJECT_NODE);
      if(node == NULL)
         return FALSE;
      pConn = node->createAgentConnection();
      if (pConn == NULL)
         return FALSE;
   }
   else
   {
      // Resolve hostname
      InetAddress addr = InetAddress::resolveHostName(pszTarget);
      if (!addr.isValid())
         return FALSE;

      Node *node = FindNodeByIP(0, addr.getAddressV4());	/* TODO: add possibility to specify action target by object id */
      if (node != NULL)
      {
         pConn = node->createAgentConnection();
         if (pConn == NULL)
            return FALSE;
      }
      else
      {
         // Target node is not in our database, try default communication settings
         pConn = new AgentConnection(addr, AGENT_LISTEN_PORT, AUTH_NONE, _T(""));
         pConn->setCommandTimeout(g_agentCommandTimeout);
         if (!pConn->connect(g_pServerKey))
         {
            pConn->decRefCount();
            return FALSE;
         }
      }
   }

   StringList *list = SplitCommandLine(pszAction);
	TCHAR *pTmp = MemCopyString(pszAction);

   UINT32 rcc = pConn->execAction(pTmp, list);
   delete list;
   pConn->disconnect();
   pConn->decRefCount();
   MemFree(pTmp);
   return rcc == ERR_SUCCESS;
}

/**
 * Run external command via system()
 */
static void RunCommand(void *arg)
{
   if (ConfigReadBoolean(_T("EscapeLocalCommands"), false))
   {
      StringBuffer s = (TCHAR *)arg;
#ifdef _WIN32
      s.replace(_T("\t"), _T("\\t"));
      s.replace(_T("\n"), _T("\\n"));
      s.replace(_T("\r"), _T("\\r"));
#else
      s.replace(_T("\t"), _T("\\\\t"));
      s.replace(_T("\n"), _T("\\\\n"));
      s.replace(_T("\r"), _T("\\\\r"));
#endif
      MemFree(arg);
      arg = MemCopyString(s.cstr());
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Executing command \"%s\""), (TCHAR *)arg);
	if (_tsystem((TCHAR *)arg) == -1)
	   nxlog_debug_tag(DEBUG_TAG, 5, _T("RunCommandThread: failed to execute command \"%s\""), (TCHAR *)arg);
	MemFree(arg);
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
		NetObj *object;

		msg.setId(1);
		msg.setCode(CMD_FORWARD_EVENT);
		object = FindObjectById(event->getSourceId());
		if (object != NULL)
		{
         if (object->getObjectClass() == OBJECT_NODE)
         {
			   msg.setField(VID_IP_ADDRESS, ((Node *)object)->getIpAddress());
         }
			msg.setField(VID_EVENT_CODE, event->getCode());
			msg.setField(VID_EVENT_NAME, event->getName());
         msg.setField(VID_TAGS, event->getTagsAsList());
			msg.setField(VID_NUM_ARGS, (WORD)event->getParametersCount());
			for(int i = 0; i < event->getParametersCount(); i++)
				msg.setField(VID_EVENT_ARG_BASE + i, event->getParameter(i));

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
static bool ExecuteActionScript(const TCHAR *scriptName, const Event *event)
{
   bool success = false;
	NXSL_VM *vm = CreateServerScriptVM(scriptName, FindObjectById(event->getSourceId()));
	if (vm != NULL)
	{
		vm->setGlobalVariable("$event", vm->createValue(new NXSL_Object(vm, &g_nxslEventClass, event, true)));

		// Pass event's parameters as arguments
		NXSL_Value **ppValueList = (NXSL_Value **)malloc(sizeof(NXSL_Value *) * event->getParametersCount());
		memset(ppValueList, 0, sizeof(NXSL_Value *) * event->getParametersCount());
		for(int i = 0; i < event->getParametersCount(); i++)
			ppValueList[i] = vm->createValue(event->getParameter(i));

		if (vm->run(event->getParametersCount(), ppValueList))
		{
			nxlog_debug_tag(DEBUG_TAG, 4, _T("ExecuteActionScript: script %s successfully executed"), scriptName);
			success = true;
		}
		else
		{
			nxlog_debug_tag(DEBUG_TAG, 4, _T("ExecuteActionScript: Script %s execution error: %s"), scriptName, vm->getErrorText());
			PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", scriptName, vm->getErrorText(), 0);
		}
	   free(ppValueList);
      delete vm;
	}
	else
	{
		nxlog_debug_tag(DEBUG_TAG, 4, _T("ExecuteActionScript(): Cannot find script %s"), scriptName);
	}
	return success;
}

/**
 * Execute action on specific event
 */
BOOL ExecuteAction(UINT32 actionId, const Event *event, const Alarm *alarm)
{
   static const TCHAR *actionType[] = { _T("EXEC"), _T("REMOTE"), _T("SEND EMAIL"), _T("SEND NOTIFICATION"), _T("FORWARD EVENT"), _T("NXSL SCRIPT"), _T("XMPP MESSAGE") };

   BOOL bSuccess = FALSE;

   shared_ptr<Action> action = s_actions.getShared(actionId);
   if (action != NULL)
   {
      if (action->isDisabled)
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Action %d (%s) is disabled and will not be executed"), actionId, action->name);
         bSuccess = TRUE;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Executing action %d (%s) of type %s"),
            actionId, action->name, actionType[action->type]);

         StringBuffer expandedData = event->expandText(CHECK_NULL_EX(action->data), alarm);
         expandedData.trim();

         StringBuffer expandedRcpt = event->expandText(action->rcptAddr, alarm);
         expandedRcpt.trim();

         switch(action->type)
         {
            case ACTION_EXEC:
               if (!expandedData.isEmpty())
               {
                  ThreadPoolExecute(g_mainThreadPool, RunCommand, MemCopyString(expandedData));
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty command - nothing to execute"));
               }
					bSuccess = TRUE;
               break;
            case ACTION_SEND_EMAIL:
               if (!expandedRcpt.isEmpty())
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Sending mail to %s: \"%s\""), (const TCHAR *)expandedRcpt, (const TCHAR *)expandedData);
                  String expandedSubject = event->expandText(action->emailSubject, alarm);
					   TCHAR *curr = expandedRcpt.getBuffer(), *next;
					   do
					   {
						   next = _tcschr(curr, _T(';'));
						   if (next != NULL)
							   *next = 0;
						   StrStrip(curr);
						   PostMail(curr, expandedSubject, expandedData);
						   curr = next + 1;
					   } while(next != NULL);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty recipients list - mail will not be sent"));
               }
               bSuccess = TRUE;
               break;
            case ACTION_NOTIFICATION:
               if (action->channelName[0] != 0)
               {
                  if(expandedRcpt.isEmpty())
                     nxlog_debug_tag(DEBUG_TAG, 3, _T("Sending notification using channel %s: \"%s\""), action->channelName, (const TCHAR *)expandedData);

                  else
                     nxlog_debug_tag(DEBUG_TAG, 3, _T("Sending notification using channel %s to %s: \"%s\""), action->channelName, (const TCHAR *)expandedRcpt, (const TCHAR *)expandedData);
                  String expandedSubject = event->expandText(action->emailSubject, alarm);
                  SendNotification(action->channelName, expandedRcpt.getBuffer(), expandedSubject, expandedData);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty channel name - notification will not be sent"));
               }
               bSuccess = TRUE;
               break;
            case ACTION_XMPP_MESSAGE:
               if (!expandedRcpt.isEmpty())
               {
#if XMPP_SUPPORTED
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Sending XMPP message to %s: \"%s\""), (const TCHAR *)expandedRcpt, (const TCHAR *)expandedData);
					   TCHAR *curr = expandedRcpt.getBuffer(), *next;
					   do
					   {
						   next = _tcschr(curr, _T(';'));
						   if (next != NULL)
							   *next = 0;
						   StrStrip(curr);
	                  SendXMPPMessage(curr, expandedData);
						   curr = next + 1;
					   } while(next != NULL);
#else
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("cannot send XMPP message to %s (server compiled without XMPP support)"), (const TCHAR *)expandedRcpt);
#endif
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty recipients list - XMPP message will not be sent"));
               }
               bSuccess = TRUE;
               break;
            case ACTION_REMOTE:
               if (!expandedRcpt.isEmpty())
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Executing on \"%s\": \"%s\""), (const TCHAR *)expandedRcpt, (const TCHAR *)expandedData);
                  bSuccess = ExecuteRemoteAction(expandedRcpt, expandedData);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty target list - remote action will not be executed"));
                  bSuccess = TRUE;
               }
               break;
				case ACTION_FORWARD_EVENT:
               if (!expandedRcpt.isEmpty())
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Forwarding event to \"%s\""), (const TCHAR *)expandedRcpt);
                  bSuccess = ForwardEvent(expandedRcpt, event);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty destination - event will not be forwarded"));
                  bSuccess = TRUE;
               }
					break;
				case ACTION_NXSL_SCRIPT:
               if (!expandedRcpt.isEmpty())
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Executing NXSL script \"%s\""), (const TCHAR *)expandedRcpt);
                  bSuccess = ExecuteActionScript(expandedRcpt, event);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 3, _T("Empty script name - nothing to execute"));
                  bSuccess = TRUE;
               }
					break;
            default:
               break;
         }
      }
   }
   return bSuccess;
}

/**
 * Action name comparator
 */
static bool ActionNameComparator(const UINT32 *id, const Action *action, const TCHAR *name)
{
   return _tcsicmp(action->name, name) == 0;
}

/**
 * Create new action
 */
UINT32 CreateAction(const TCHAR *name, UINT32 *id)
{
   // Check for duplicate name
   if (s_actions.findElement(ActionNameComparator, name) != NULL)
      return RCC_OBJECT_ALREADY_EXISTS;

   Action *action = new Action(name);
   *id = action->id;
   action->saveToDatabase();

   s_actions.set(action->id, action);
   m_dwUpdateCode = NX_NOTIFY_ACTION_CREATED;
   EnumerateClientSessions(SendActionDBUpdate, action);

   return RCC_SUCCESS;
}

/**
 * Delete action
 */
UINT32 DeleteAction(UINT32 actionId)
{
   UINT32 dwResult = RCC_SUCCESS;

   shared_ptr<Action> action = s_actions.getShared(actionId);
   if (action != NULL)
   {
      m_dwUpdateCode = NX_NOTIFY_ACTION_DELETED;
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
UINT32 ModifyActionFromMessage(NXCPMessage *pMsg)
{
   UINT32 dwResult = RCC_INVALID_ACTION_ID;
	TCHAR name[MAX_OBJECT_NAME];

   pMsg->getFieldAsString(VID_ACTION_NAME, name, MAX_OBJECT_NAME);
	if (!IsValidObjectName(name, TRUE))
		return RCC_INVALID_OBJECT_NAME;

   UINT32 actionId = pMsg->getFieldAsUInt32(VID_ACTION_ID);

   shared_ptr<Action> tmp = s_actions.getShared(actionId);
   if (tmp != NULL)
   {
      Action *action = new Action(tmp.get());
      action->isDisabled = pMsg->getFieldAsBoolean(VID_IS_DISABLED);
      action->type = pMsg->getFieldAsUInt16(VID_ACTION_TYPE);
      free(action->data);
      action->data = pMsg->getFieldAsString(VID_ACTION_DATA);
      pMsg->getFieldAsString(VID_EMAIL_SUBJECT, action->emailSubject, MAX_EMAIL_SUBJECT_LEN);
      pMsg->getFieldAsString(VID_RCPT_ADDR, action->rcptAddr, MAX_RCPT_ADDR_LEN);
      pMsg->getFieldAsString(VID_CHANNEL_NAME, action->channelName, MAX_OBJECT_NAME);
      _tcscpy(action->name, name);

      action->saveToDatabase();

      m_dwUpdateCode = NX_NOTIFY_ACTION_MODIFIED;
      EnumerateClientSessions(SendActionDBUpdate, action);
      s_actions.set(actionId, action);

      dwResult = RCC_SUCCESS;
   }

   return dwResult;
}

/**
 * Send action to client
 * first - old name
 * second - new name
 */
static EnumerationCallbackResult RenameChannel(const UINT32 *id, const Action *action, std::pair<TCHAR *, TCHAR *> *names)
{
   if (!_tcsncmp(action->channelName, names->first, MAX_OBJECT_NAME) && action->type == ACTION_NOTIFICATION)
   {
      _tcsncpy(const_cast<TCHAR *>(action->channelName), names->second, MAX_OBJECT_NAME);
      m_dwUpdateCode = NX_NOTIFY_ACTION_MODIFIED;
      EnumerateClientSessions(SendActionDBUpdate, const_cast<Action *>(action));
   }
   return _CONTINUE;
}

/**
 * Update notification channel name in all actions
 * first - old name
 * second - new name
 */
void UpdateChannelNameInActions(std::pair<TCHAR *, TCHAR *> *names)
{
   s_actions.forEach(RenameChannel, names);
}

/**
 * Check if notification channel with given name is used in actions
 */
static EnumerationCallbackResult CheckChannelUsage(const UINT32 *id, const Action *action, TCHAR *name)
{
   if(!_tcsncmp(action->channelName, name, MAX_OBJECT_NAME))
   {
      return _STOP;
   }
   return _CONTINUE;
}

/**
 * Check if notification channel with given name is used in actions
 */
bool CheckChannelIsUsedInAction(TCHAR *name)
{
   return s_actions.forEach(CheckChannelUsage, name) == _STOP;
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
static EnumerationCallbackResult SendAction(const UINT32 *id, const Action *action, SendActionData *data)
{
   action->fillMessage(data->message);
   data->session->sendMessage(static_cast<SendActionData*>(data)->message);
   data->message->deleteAllFields();

   return _CONTINUE;
}

/**
 * Send all actions to client
 */
void SendActionsToClient(ClientSession *session, UINT32 requestId)
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
void CreateActionExportRecord(StringBuffer &xml, UINT32 id)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT guid,action_name,action_type,")
                                       _T("rcpt_addr,email_subject,action_data,")
                                       _T("channel_name FROM actions WHERE action_id=?"));
   if (hStmt == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         xml.append(_T("\t\t<action id=\""));
         xml.append(id);
         xml.append(_T("\">\n\t\t\t<guid>"));
         xml.append(DBGetFieldGUID(hResult, 0, 0));
         xml.append(_T("</guid>\n\t\t\t<name>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 1));
         xml.append(_T("</name>\n\t\t\t<type>"));
         xml.append(DBGetFieldULong(hResult, 0, 2));
         xml.append(_T("</type>\n\t\t\t<recipientAddress>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 3));
         xml.append(_T("</recipientAddress>\n\t\t\t<emailSubject>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 4));
         xml.append(_T("</emailSubject>\n\t\t\t<data>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 5));
         xml.append(_T("</data>\n\t\t\t<channelName>"));
         xml.appendPreallocated(DBGetFieldForXML(hResult, 0, 6));
         xml.append(_T("</channelName>\n"));
         xml.append(_T("\t\t</action>\n"));
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Check if given action ID is valid
 */
bool IsValidActionId(UINT32 id)
{
   return s_actions.getShared(id) != NULL;
}

/**
 * Get GUID of given action
 */
uuid GetActionGUID(UINT32 id)
{
   shared_ptr<Action> action = s_actions.getShared(id);
   uuid guid = (action != NULL) ? action->guid : uuid::NULL_UUID;
   return guid;
}

/**
 * Action GUID comparator
 */
static bool ActionGUIDComparator(const UINT32 *id, const Action *action, const uuid *data)
{
   return action->guid.equals(*data);
}

/**
 * Find action by GUID
 */
UINT32 FindActionByGUID(const uuid& guid)
{
   shared_ptr<Action> action = s_actions.findElement(ActionGUIDComparator, &guid);
   UINT32 id = (action != NULL) ? action->id : 0;
   return id;
}

/**
 * Import action configuration
 */
bool ImportAction(ConfigEntry *config, bool overwrite)
{
   if (config->getSubEntryValue(_T("name")) == NULL)
   {
      nxlog_debug_tag(_T("import"), 4, _T("ImportAction: no name specified"));
      return false;
   }

   const uuid guid = config->getSubEntryValueAsUUID(_T("guid"));
   shared_ptr<Action> action = !guid.isNull() ? s_actions.findElement(ActionGUIDComparator, &guid) : shared_ptr<Action>();
   if (action == NULL)
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
      m_dwUpdateCode = NX_NOTIFY_ACTION_CREATED;
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
      m_dwUpdateCode = NX_NOTIFY_ACTION_MODIFIED;
      action = make_shared<Action>(action.get());
   }

   // If not exist, create it
   action->type = config->getSubEntryValueAsUInt(_T("type"), 0);
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
void ExecuteScheduledAction(const ScheduledTaskParameters *parameter)
{
   UINT32 actionId = ExtractNamedOptionValueAsUInt(parameter->m_persistentData, _T("action"), 0);
   Event *restoredEvent = NULL;
   Alarm *restoredAlarm = NULL;
   const Event *event;
   const Alarm *alarm;
   if (parameter->m_transientData != NULL)
   {
      event = static_cast<ActionExecutionTransientData*>(parameter->m_transientData)->getEvent();
      alarm = static_cast<ActionExecutionTransientData*>(parameter->m_transientData)->getAlarm();
   }
   else
   {
      UINT64 eventId = ExtractNamedOptionValueAsUInt64(parameter->m_persistentData, _T("event"), 0);
      if (eventId != 0)
      {
         restoredEvent = LoadEventFromDatabase(eventId);
      }

      UINT32 alarmId = ExtractNamedOptionValueAsUInt(parameter->m_persistentData, _T("alarm"), 0);
      if (alarmId != 0)
      {
         restoredAlarm = FindAlarmById(alarmId);
         if (restoredAlarm == NULL)
            restoredAlarm = LoadAlarmFromDatabase(alarmId);
      }

      event = restoredEvent;
      alarm = restoredAlarm;
   }

   if (event != NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Executing scheduled action [%u] for event %s on node [%u]"),
               actionId, event->getName(), parameter->m_objectId);
      ExecuteAction(actionId, event, alarm);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot execute scheduled action [%u] on node [%u]: original event is unavailable"),
               actionId, parameter->m_objectId);
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
   m_alarm = (a != NULL) ? new Alarm(a, false) : NULL;
}

/**
 * Destructor for scheduled action execution transient data
 */
ActionExecutionTransientData::~ActionExecutionTransientData()
{
   delete m_event;
   delete m_alarm;
}
