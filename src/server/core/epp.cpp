/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: epp.cpp
**
**/

#include "nxcore.h"

/**
 * Default event policy rule constructor
 */
EPRule::EPRule(UINT32 id)
{
   m_id = id;
   m_guid = uuid::generate();
   m_dwFlags = 0;
   m_dwNumSources = 0;
   m_pdwSourceList = NULL;
   m_dwNumEvents = 0;
   m_pdwEventList = NULL;
   m_dwNumActions = 0;
   m_pdwActionList = NULL;
   m_pszComment = NULL;
   m_iAlarmSeverity = 0;
   m_szAlarmKey[0] = 0;
   m_szAlarmMessage[0] = 0;
   m_pszScript = NULL;
   m_pScript = NULL;
	m_dwAlarmTimeout = 0;
	m_dwAlarmTimeoutEvent = EVENT_ALARM_TIMEOUT;
	m_alarmCategoryList = new IntegerArray<UINT32>(16, 16);
}

/**
 * Create rule from config entry
 */
EPRule::EPRule(ConfigEntry *config)
{
   m_id = 0;
   m_guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (m_guid.isNull())
      m_guid = uuid::generate(); // generate random GUID if rule was imported without GUID
   m_dwFlags = config->getSubEntryValueAsUInt(_T("flags"));
   m_dwNumSources = 0;
   m_pdwSourceList = NULL;
   m_dwNumActions = 0;
   m_pdwActionList = NULL;

	ConfigEntry *eventsRoot = config->findEntry(_T("events"));
   if (eventsRoot != NULL)
   {
		ObjectArray<ConfigEntry> *events = eventsRoot->getSubEntries(_T("event#*"));
      m_dwNumEvents = 0;
      m_pdwEventList = (UINT32 *)malloc(sizeof(UINT32) * events->size());
      for(int i = 0; i < events->size(); i++)
      {
         EventTemplate *e = FindEventTemplateByName(events->get(i)->getSubEntryValue(_T("name"), 0, _T("<unknown>")));
         if (e != NULL)
         {
            m_pdwEventList[m_dwNumEvents++] = e->getCode();
            e->decRefCount();
         }
      }
   }

   m_pszComment = _tcsdup(config->getSubEntryValue(_T("comments"), 0, _T("")));
   m_iAlarmSeverity = config->getSubEntryValueAsInt(_T("alarmSeverity"));
	m_dwAlarmTimeout = config->getSubEntryValueAsUInt(_T("alarmTimeout"));
	m_dwAlarmTimeoutEvent = config->getSubEntryValueAsUInt(_T("alarmTimeoutEvent"), 0, EVENT_ALARM_TIMEOUT);
	m_alarmCategoryList = new IntegerArray<UINT32>(16, 16);
   nx_strncpy(m_szAlarmKey, config->getSubEntryValue(_T("alarmKey"), 0, _T("")), MAX_DB_STRING);
   nx_strncpy(m_szAlarmMessage, config->getSubEntryValue(_T("alarmMessage"), 0, _T("")), MAX_DB_STRING);

   ConfigEntry *pStorageEntry = config->findEntry(_T("pStorageActions"));
   if (pStorageEntry != NULL)
   {
      ObjectArray<ConfigEntry> *tmp = pStorageEntry->getSubEntries(_T("setValue"));
      if(tmp->size() > 0)
      {
         tmp = tmp->get(0)->getSubEntries(_T("value"));
         for(int i = 0; i < tmp->size(); i++)
         {
            m_pstorageSetActions.set(tmp->get(i)->getAttribute(_T("key")), tmp->get(i)->getValue());
         }
      }
      tmp = pStorageEntry->getSubEntries(_T("deleteValue"));
      if(tmp->size() > 0)
      {
         tmp = tmp->get(0)->getSubEntries(_T("value"));
         for(int i = 0; i < tmp->size(); i++)
         {
            m_pstorageDeleteActions.add(tmp->get(i)->getAttribute(_T("key")));
         }

      }
   }

   m_pszScript = _tcsdup(config->getSubEntryValue(_T("script"), 0, _T("")));
   if ((m_pszScript != NULL) && (*m_pszScript != 0))
   {
      TCHAR szError[256];

      m_pScript = NXSLCompileAndCreateVM(m_pszScript, szError, 256, new NXSL_ServerEnv);
      if (m_pScript != NULL)
      {
      	m_pScript->setGlobalVariable(_T("CUSTOM_MESSAGE"), new NXSL_Value(_T("")));
      }
      else
      {
         nxlog_write(MSG_EPRULE_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE, "ds", m_id, szError);
      }
   }
   else
   {
      m_pScript = NULL;
   }
}

/**
 * Construct event policy rule from database record
 * Assuming the following field order:
 * rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,
 * alarm_timeout,alarm_timeout_event
 */
EPRule::EPRule(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_guid = DBGetFieldGUID(hResult, row, 1);
   m_dwFlags = DBGetFieldULong(hResult, row, 2);
   m_pszComment = DBGetField(hResult, row, 3, NULL, 0);
	DBGetField(hResult, row, 4, m_szAlarmMessage, MAX_EVENT_MSG_LENGTH);
   m_iAlarmSeverity = DBGetFieldLong(hResult, row, 5);
   DBGetField(hResult, row, 6, m_szAlarmKey, MAX_DB_STRING);
   m_pszScript = DBGetField(hResult, row, 7, NULL, 0);
   if ((m_pszScript != NULL) && (*m_pszScript != 0))
   {
      TCHAR szError[256];

      m_pScript = NXSLCompileAndCreateVM(m_pszScript, szError, 256, new NXSL_ServerEnv);
      if (m_pScript != NULL)
      {
      	m_pScript->setGlobalVariable(_T("CUSTOM_MESSAGE"), new NXSL_Value(_T("")));
      }
      else
      {
         nxlog_write(MSG_EPRULE_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE,
                  "ds", m_id, szError);
      }
   }
   else
   {
      m_pScript = NULL;
   }
	m_dwAlarmTimeout = DBGetFieldULong(hResult, row, 8);
	m_dwAlarmTimeoutEvent = DBGetFieldULong(hResult, row, 9);
	m_alarmCategoryList = new IntegerArray<UINT32>(16, 16);
   m_dwNumActions = 0;
   m_pdwActionList = NULL;
   m_dwNumEvents = 0;
   m_pdwEventList = NULL;
   m_dwNumSources = 0;
   m_pdwSourceList = NULL;
}

/**
 * Construct event policy rule from NXCP message
 */
EPRule::EPRule(NXCPMessage *msg)
{
   m_dwFlags = msg->getFieldAsUInt32(VID_FLAGS);
   m_id = msg->getFieldAsUInt32(VID_RULE_ID);
   m_guid = msg->getFieldAsGUID(VID_GUID);
   m_pszComment = msg->getFieldAsString(VID_COMMENTS);

   m_dwNumActions = msg->getFieldAsUInt32(VID_NUM_ACTIONS);
   m_pdwActionList = (UINT32 *)malloc(sizeof(UINT32) * m_dwNumActions);
   msg->getFieldAsInt32Array(VID_RULE_ACTIONS, m_dwNumActions, m_pdwActionList);

   m_dwNumEvents = msg->getFieldAsUInt32(VID_NUM_EVENTS);
   m_pdwEventList = (UINT32 *)malloc(sizeof(UINT32) * m_dwNumEvents);
   msg->getFieldAsInt32Array(VID_RULE_EVENTS, m_dwNumEvents, m_pdwEventList);

   m_dwNumSources = msg->getFieldAsUInt32(VID_NUM_SOURCES);
   m_pdwSourceList = (UINT32 *)malloc(sizeof(UINT32) * m_dwNumSources);
   msg->getFieldAsInt32Array(VID_RULE_SOURCES, m_dwNumSources, m_pdwSourceList);

   msg->getFieldAsString(VID_ALARM_KEY, m_szAlarmKey, MAX_DB_STRING);
   msg->getFieldAsString(VID_ALARM_MESSAGE, m_szAlarmMessage, MAX_DB_STRING);
   m_iAlarmSeverity = msg->getFieldAsUInt16(VID_ALARM_SEVERITY);
	m_dwAlarmTimeout = msg->getFieldAsUInt32(VID_ALARM_TIMEOUT);
	m_dwAlarmTimeoutEvent = msg->getFieldAsUInt32(VID_ALARM_TIMEOUT_EVENT);

   m_alarmCategoryList = new IntegerArray<UINT32>(16, 16);
   msg->getFieldAsInt32Array(VID_ALARM_CATEGORY_ID, m_alarmCategoryList);

   int count = msg->getFieldAsInt32(VID_NUM_SET_PSTORAGE);
   int base = VID_PSTORAGE_SET_LIST_BASE;
   for(int i = 0; i < count; i++, base+=2)
   {
      m_pstorageSetActions.setPreallocated(msg->getFieldAsString(base), msg->getFieldAsString(base+1));
   }

   count = msg->getFieldAsInt32(VID_NUM_DELETE_PSTORAGE);
   base = VID_PSTORAGE_DELETE_LIST_BASE;
   for(int i = 0; i < count; i++, base++)
   {
      m_pstorageDeleteActions.addPreallocated(msg->getFieldAsString(base));
   }

   m_pszScript = msg->getFieldAsString(VID_SCRIPT);
   if ((m_pszScript != NULL) && (*m_pszScript != 0))
   {
      TCHAR szError[256];

      m_pScript = NXSLCompileAndCreateVM(m_pszScript, szError, 256, new NXSL_ServerEnv);
      if (m_pScript != NULL)
      {
      	m_pScript->setGlobalVariable(_T("CUSTOM_MESSAGE"), new NXSL_Value(_T("")));
      }
      else
      {
         nxlog_write(MSG_EPRULE_SCRIPT_COMPILATION_ERROR, EVENTLOG_ERROR_TYPE, "ds", m_id, szError);
      }
   }
   else
   {
      m_pScript = NULL;
   }
}

/**
 * Event policy rule destructor
 */
EPRule::~EPRule()
{
   free(m_pdwSourceList);
   free(m_pdwEventList);
   free(m_pdwActionList);
   free(m_pszComment);
   free(m_pszScript);
   delete m_alarmCategoryList;
   delete m_pScript;
}

/**
 * Callback for filling export XML with set persistent storage actions
 */
static EnumerationCallbackResult AddPSSetActionsToSML(const TCHAR *key, const void *value, void *data)
{
   String *str = (String *)data;
   str->appendFormattedString(_T("\t\t\t\t\t<value key=\"%d\">%s</value>\n"), key, value);
   return _CONTINUE;
}

/**
 * Create management pack record
 */
void EPRule::createNXMPRecord(String &str)
{
   str.append(_T("\t\t<rule id=\""));
   str.append(m_id + 1);
   str.append(_T("\">\n\t\t\t<guid>"));
   str.append(m_guid.toString());
   str.append(_T("</guid>\n\t\t\t<flags>"));
   str.append(m_dwFlags);
   str.append(_T("</flags>\n\t\t\t<alarmMessage>"));
   str.append(EscapeStringForXML2(m_szAlarmMessage));
   str.append(_T("</alarmMessage>\n\t\t\t<alarmKey>"));
   str.append(EscapeStringForXML2(m_szAlarmKey));
   str.append(_T("</alarmKey>\n\t\t\t<alarmSeverity>"));
   str.append(m_iAlarmSeverity);
   str.append(_T("</alarmSeverity>\n\t\t\t<alarmTimeout>"));
   str.append(m_dwAlarmTimeout);
   str.append(_T("</alarmTimeout>\n\t\t\t<alarmTimeoutEvent>"));
   str.append(m_dwAlarmTimeoutEvent);
   str.append(_T("</alarmTimeoutEvent>\n\t\t\t<script>"));
   str.append(EscapeStringForXML2(m_pszScript));
   str.append(_T("</script>\n\t\t\t<comments>"));
   str.append(EscapeStringForXML2(m_pszComment));
   str.append(_T("</comments>\n\t\t\t<sources>\n"));

   for(UINT32 i = 0; i < m_dwNumSources; i++)
   {
      NetObj *object = FindObjectById(m_pdwSourceList[i]);
      if (object != NULL)
      {
         TCHAR guidText[128];
         str.appendFormattedString(_T("\t\t\t\t<source id=\"%d\">\n")
                                _T("\t\t\t\t\t<name>%s</name>\n")
                                _T("\t\t\t\t\t<guid>%s</guid>\n")
                                _T("\t\t\t\t\t<class>%d</class>\n")
                                _T("\t\t\t\t</source>\n"),
                                object->getId(),
                                (const TCHAR *)EscapeStringForXML2(object->getName()),
                                object->getGuid().toString(guidText), object->getObjectClass());
      }
   }

   str += _T("\t\t\t</sources>\n\t\t\t<events>\n");

   for(UINT32 i = 0; i < m_dwNumEvents; i++)
   {
      TCHAR eventName[MAX_EVENT_NAME];
      EventNameFromCode(m_pdwEventList[i], eventName);
      str.appendFormattedString(_T("\t\t\t\t<event id=\"%d\">\n")
                             _T("\t\t\t\t\t<name>%s</name>\n")
                             _T("\t\t\t\t</event>\n"),
                             m_pdwEventList[i], (const TCHAR *)EscapeStringForXML2(eventName));
   }

   str += _T("\t\t\t</events>\n\t\t\t<actions>\n");

   for(UINT32 i = 0; i < m_dwNumActions; i++)
   {
      str.appendFormattedString(_T("\t\t\t\t<action id=\"%d\">\n")
                             _T("\t\t\t\t</action>\n"),
                             m_pdwActionList[i]);
   }

   str += _T("\t\t\t</actions>\n\t\t\t<pStorageActions>\n\t\t\t\t<setValue>\n");
   m_pstorageSetActions.forEach(AddPSSetActionsToSML, &str);
   str += _T("\t\t\t\t</setValue>\n\t\t\t\t<deleteValue>\n");
   for(int i = 0; i < m_pstorageDeleteActions.size(); i++)
      str.appendFormattedString(_T("\t\t\t\t\t<value key=\"%s\">\n"), m_pstorageDeleteActions.get(i));
   str += _T("\t\t\t\t</deleteValue>\n\t\t\t</pStorageActions>\n\t\t</rule>\n");
}

/**
 * Check if source object's id match to the rule
 */
bool EPRule::matchSource(UINT32 dwObjectId)
{
   UINT32 i;
   NetObj *pObject;
   bool bMatch = FALSE;

   if (m_dwNumSources == 0)   // No sources in list means "any"
   {
      bMatch = true;
   }
   else
   {
      for(i = 0; i < m_dwNumSources; i++)
         if (m_pdwSourceList[i] == dwObjectId)
         {
            bMatch = true;
            break;
         }
         else
         {
            pObject = FindObjectById(m_pdwSourceList[i]);
            if (pObject != NULL)
            {
               if (pObject->isChild(dwObjectId))
               {
                  bMatch = true;
                  break;
               }
            }
            else
            {
               nxlog_write(MSG_INVALID_EPP_OBJECT, EVENTLOG_ERROR_TYPE, "d", m_pdwSourceList[i]);
            }
         }
   }
   return (m_dwFlags & RF_NEGATED_SOURCE) ? !bMatch : bMatch;
}

/**
 * Check if event's id match to the rule
 */
bool EPRule::matchEvent(UINT32 dwEventCode)
{
   UINT32 i;
   bool bMatch = false;

   if (m_dwNumEvents == 0)   // No sources in list means "any"
   {
      bMatch = true;
   }
   else
   {
      for(i = 0; i < m_dwNumEvents; i++)
         if (m_pdwEventList[i] & GROUP_FLAG_BIT)
         {
            /* TODO: check group membership */
         }
         else
         {
            if (m_pdwEventList[i] == dwEventCode)
            {
               bMatch = true;
               break;
            }
         }
   }
   return (m_dwFlags & RF_NEGATED_EVENTS) ? !bMatch : bMatch;
}

/**
 * Check if event's severity match to the rule
 */
bool EPRule::matchSeverity(UINT32 dwSeverity)
{
   static UINT32 dwSeverityFlag[] = { RF_SEVERITY_INFO, RF_SEVERITY_WARNING,
                                     RF_SEVERITY_MINOR, RF_SEVERITY_MAJOR,
                                     RF_SEVERITY_CRITICAL };
	return (dwSeverityFlag[dwSeverity] & m_dwFlags) ? true : false;
}

/**
 * Check if event match to the script
 */
bool EPRule::matchScript(Event *pEvent)
{
   bool bRet = true;

   if (m_pScript == NULL)
      return true;

   // Pass event's parameters as arguments and
   // other information as variables
   NXSL_Value **ppValueList = (NXSL_Value **)malloc(sizeof(NXSL_Value *) * pEvent->getParametersCount());
   memset(ppValueList, 0, sizeof(NXSL_Value *) * pEvent->getParametersCount());
   for(int i = 0; i < pEvent->getParametersCount(); i++)
      ppValueList[i] = new NXSL_Value(pEvent->getParameter(i));

   NXSL_VariableSystem *pLocals = new NXSL_VariableSystem;
   pLocals->create(_T("EVENT_CODE"), new NXSL_Value(pEvent->getCode()));
   pLocals->create(_T("SEVERITY"), new NXSL_Value(pEvent->getSeverity()));
   pLocals->create(_T("SEVERITY_TEXT"), new NXSL_Value(GetStatusAsText(pEvent->getSeverity(), true)));
   pLocals->create(_T("OBJECT_ID"), new NXSL_Value(pEvent->getSourceId()));
   pLocals->create(_T("EVENT_TEXT"), new NXSL_Value((TCHAR *)pEvent->getMessage()));
   pLocals->create(_T("USER_TAG"), new NXSL_Value((TCHAR *)pEvent->getUserTag()));
	NetObj *pObject = FindObjectById(pEvent->getSourceId());
	if (pObject != NULL)
	{
      m_pScript->setGlobalVariable(_T("$object"), pObject->createNXSLObject());
		if (pObject->getObjectClass() == OBJECT_NODE)
			m_pScript->setGlobalVariable(_T("$node"), pObject->createNXSLObject());
	}
	m_pScript->setGlobalVariable(_T("$event"), new NXSL_Value(new NXSL_Object(&g_nxslEventClass, pEvent)));
	m_pScript->setGlobalVariable(_T("CUSTOM_MESSAGE"), new NXSL_Value);

   // Run script
   NXSL_VariableSystem *globals = NULL;
   if (m_pScript->run(pEvent->getParametersCount(), ppValueList, pLocals, &globals))
   {
      NXSL_Value *value = m_pScript->getResult();
      if (value != NULL)
      {
         bRet = value->getValueAsInt32() ? true : false;
         if (bRet)
         {
         	NXSL_Variable *var = globals->find(_T("CUSTOM_MESSAGE"));
         	if (var != NULL)
         	{
         		// Update custom message in event
         		pEvent->setCustomMessage(CHECK_NULL_EX(var->getValue()->getValueAsCString()));
         	}
         }
      }
   }
   else
   {
      TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("EPP::%d"), m_id + 1);
      PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, m_pScript->getErrorText(), 0);
      nxlog_write(MSG_EPRULE_SCRIPT_EXECUTION_ERROR, EVENTLOG_ERROR_TYPE, "ds", m_id + 1, m_pScript->getErrorText());
   }
   free(ppValueList);
   delete globals;

   return bRet;
}

/**
 * Callback for execution set action on persistent storage
 */
static EnumerationCallbackResult ExecutePstorageSetAction(const TCHAR *key, const void *value, void *data)
{
   Event *pEvent = (Event *)data;
   TCHAR *psValue = pEvent->expandText(key);
   TCHAR *psKey = pEvent->expandText((TCHAR *)value);
   SetPersistentStorageValue(psValue, psKey);
   free(psValue);
   free(psKey);
   return _CONTINUE;
}

/**
 * Check if event match to rule and perform required actions if yes
 * Method will return TRUE if event matched and RF_STOP_PROCESSING flag is set
 */
bool EPRule::processEvent(Event *pEvent)
{
   bool bStopProcessing = false;

   // Check disable flag
   if (!(m_dwFlags & RF_DISABLED))
   {
      // Check if event match
      if (matchSource(pEvent->getSourceId()) && matchEvent(pEvent->getCode()) &&
          matchSeverity(pEvent->getSeverity()) && matchScript(pEvent))
      {
			DbgPrintf(6, _T("Event ") UINT64_FMT _T(" match EPP rule %d"), pEvent->getId(), (int)m_id);

         // Generate alarm if requested
         if (m_dwFlags & RF_GENERATE_ALARM)
            generateAlarm(pEvent);

         // Event matched, perform actions
         if (m_dwNumActions > 0)
         {
            TCHAR *alarmMessage = pEvent->expandText(m_szAlarmMessage);
            TCHAR *alarmKey = pEvent->expandText(m_szAlarmKey);
            for(UINT32 i = 0; i < m_dwNumActions; i++)
               ExecuteAction(m_pdwActionList[i], pEvent, alarmMessage, alarmKey);
            free(alarmMessage);
            free(alarmKey);
         }

			// Update persistent storage if needed
			if (m_pstorageSetActions.size() > 0)
            m_pstorageSetActions.forEach(ExecutePstorageSetAction, pEvent);
			for(int i = 0; i < m_pstorageDeleteActions.size(); i++)
         {
            TCHAR *psKey = pEvent->expandText(m_pstorageDeleteActions.get(i));
            DeletePersistentStorageValue(psKey);
            free(psKey);
         }

			bStopProcessing = (m_dwFlags & RF_STOP_PROCESSING) ? true : false;
      }
   }

   return bStopProcessing;
}

/**
 * Generate alarm from event
 */
void EPRule::generateAlarm(Event *pEvent)
{
   // Terminate alarms with key == our ack_key
	if ((m_iAlarmSeverity == SEVERITY_RESOLVE) || (m_iAlarmSeverity == SEVERITY_TERMINATE))
	{
		TCHAR *pszAckKey = pEvent->expandText(m_szAlarmKey);
		if (pszAckKey[0] != 0)
			ResolveAlarmByKey(pszAckKey, (m_dwFlags & RF_TERMINATE_BY_REGEXP) ? true : false, m_iAlarmSeverity == SEVERITY_TERMINATE, pEvent);
		free(pszAckKey);
	}
	else	// Generate new alarm
	{
		CreateNewAlarm(m_szAlarmMessage, m_szAlarmKey, ALARM_STATE_OUTSTANDING,
                     (m_iAlarmSeverity == SEVERITY_FROM_EVENT) ? pEvent->getSeverity() : m_iAlarmSeverity,
                     m_dwAlarmTimeout, m_dwAlarmTimeoutEvent, pEvent, 0, m_alarmCategoryList,
                     ((m_dwFlags & RF_CREATE_TICKET) != 0) ? true : false);
	}
}

/**
 * Load rule from database
 */
bool EPRule::loadFromDB(DB_HANDLE hdb)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   bool bSuccess = true;
   UINT32 i, count;

   // Load rule's sources
   _sntprintf(szQuery, 256, _T("SELECT object_id FROM policy_source_list WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      m_dwNumSources = DBGetNumRows(hResult);
      m_pdwSourceList = (UINT32 *)malloc(sizeof(UINT32) * m_dwNumSources);
      for(i = 0; i < m_dwNumSources; i++)
         m_pdwSourceList[i] = DBGetFieldULong(hResult, i, 0);
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = false;
   }

   // Load rule's events
   _sntprintf(szQuery, 256, _T("SELECT event_code FROM policy_event_list WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      m_dwNumEvents = DBGetNumRows(hResult);
      m_pdwEventList = (UINT32 *)malloc(sizeof(UINT32) * m_dwNumEvents);
      for(i = 0; i < m_dwNumEvents; i++)
         m_pdwEventList[i] = DBGetFieldULong(hResult, i, 0);
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = false;
   }

   // Load rule's actions
   _sntprintf(szQuery, 256, _T("SELECT action_id FROM policy_action_list WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      m_dwNumActions = DBGetNumRows(hResult);
      m_pdwActionList = (UINT32 *)malloc(sizeof(UINT32) * m_dwNumActions);
      for(i = 0; i < m_dwNumActions; i++)
         m_pdwActionList[i] = DBGetFieldULong(hResult, i, 0);
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = false;
   }

   // Load pstorage actions
   _sntprintf(szQuery, 256, _T("SELECT ps_key,action,value FROM policy_pstorage_actions WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      TCHAR key[MAX_DB_STRING];
      count = DBGetNumRows(hResult);
      for(i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, key, MAX_DB_STRING);
         if(DBGetFieldULong(hResult, i, 1) == PSTORAGE_SET)
         {
            m_pstorageSetActions.setPreallocated(_tcsdup(key), DBGetField(hResult, i, 2, NULL, 0));
         }
         if(DBGetFieldULong(hResult, i, 1) == PSTORAGE_DELETE)
         {
            m_pstorageDeleteActions.add(key);
         }
      }
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = false;
   }

   // Load alarm categories
   _sntprintf(szQuery, 256, _T("SELECT category_id FROM alarm_category_map WHERE alarm_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      count = DBGetNumRows(hResult);
      for(i = 0; i < count; i++)
      {
         m_alarmCategoryList->add(DBGetFieldULong(hResult, i, 0));
      }
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = false;
   }

   return bSuccess;
}

/**
 * Callback for persistent storage set actions
 */
static EnumerationCallbackResult SavePstorageSetActions(const TCHAR *key, const void *value, void *data)
{
   DB_STATEMENT hStmt = (DB_STATEMENT)data;
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, (TCHAR *)value, DB_BIND_STATIC);
   return DBExecute(hStmt) ? _CONTINUE : _STOP;
}

/**
 * Save rule to database
 */
bool EPRule::saveToDB(DB_HANDLE hdb)
{
   bool success;
	int i;
	TCHAR pszQuery[1024];
   // General attributes
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,")
                                  _T("alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event)")
                                  _T("VALUES (?,?,?,?,?,?,?,?,?,?)"));
   if(hStmt != NULL)
   {
      TCHAR guidText[128];
      DBBind(hStmt, 1, DB_CTYPE_INT32, m_id);
      DBBind(hStmt, 2, DB_CTYPE_STRING, m_guid.toString(guidText), DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_CTYPE_INT32, m_dwFlags);
      DBBind(hStmt, 4, DB_CTYPE_STRING,  m_pszComment, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_CTYPE_STRING, m_szAlarmMessage, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_CTYPE_INT32, m_iAlarmSeverity);
      DBBind(hStmt, 7, DB_CTYPE_STRING, m_szAlarmKey, DB_BIND_STATIC);
      DBBind(hStmt, 8, DB_CTYPE_STRING, m_pszScript, DB_BIND_STATIC);
      DBBind(hStmt, 9, DB_CTYPE_INT32, m_dwAlarmTimeout);
      DBBind(hStmt, 10, DB_CTYPE_INT32, m_dwAlarmTimeoutEvent);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   else
   {
      success = false;
   }
   // Actions
   if(success)
   {
      for(i = 0; i < (int)m_dwNumActions && success; i++)
      {
         _sntprintf(pszQuery, 1024, _T("INSERT INTO policy_action_list (rule_id,action_id) VALUES (%d,%d)"),
                   m_id, m_pdwActionList[i]);
         success = DBQuery(hdb, pszQuery);
      }
   }

   // Events
   if(success)
   {
      for(i = 0; i < (int)m_dwNumEvents && success; i++)
      {
         _sntprintf(pszQuery, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"),
                   m_id, m_pdwEventList[i]);
         success = DBQuery(hdb, pszQuery);
      }
   }

   // Sources
   if(success)
   {
      for(i = 0; i < (int)m_dwNumSources && success; i++)
      {
         _sntprintf(pszQuery, 1024, _T("INSERT INTO policy_source_list (rule_id,object_id) VALUES (%d,%d)"),
                   m_id, m_pdwSourceList[i]);
         success = DBQuery(hdb, pszQuery);
      }
   }

	// Persistent storage actions
   if(success)
   {
      if (m_pstorageSetActions.size() > 0)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO policy_pstorage_actions (rule_id,action,ps_key,value) VALUES (?,1,?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            success = _STOP != m_pstorageSetActions.forEach(SavePstorageSetActions, hStmt);
            DBFreeStatement(hStmt);
         }
      }
   }

   if(success)
   {
      if(m_pstorageDeleteActions.size() > 0)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO policy_pstorage_actions (rule_id,action,ps_key) VALUES (?,2,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; i < m_pstorageDeleteActions.size() && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_pstorageDeleteActions.get(i), DB_BIND_STATIC);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
      }
   }

   // Alarm categories
   if(success)
   {
      if (m_alarmCategoryList->size() > 0)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO alarm_category_map (alarm_id,category_id) VALUES (?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER && success, m_id);
            for (i = 0; i < m_alarmCategoryList->size(); i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_alarmCategoryList->get(i));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
      }
   }

   return success;
}

/**
 * Create NXCP message with rule's data
 */
void EPRule::createMessage(NXCPMessage *msg)
{
   msg->setField(VID_FLAGS, m_dwFlags);
   msg->setField(VID_RULE_ID, m_id);
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_ALARM_SEVERITY, (WORD)m_iAlarmSeverity);
   msg->setField(VID_ALARM_KEY, m_szAlarmKey);
   msg->setField(VID_ALARM_MESSAGE, m_szAlarmMessage);
   msg->setField(VID_ALARM_TIMEOUT, m_dwAlarmTimeout);
   msg->setField(VID_ALARM_TIMEOUT_EVENT, m_dwAlarmTimeoutEvent);
   msg->setFieldFromInt32Array(VID_ALARM_CATEGORY_ID, m_alarmCategoryList);
   msg->setField(VID_COMMENTS, CHECK_NULL_EX(m_pszComment));
   msg->setField(VID_NUM_ACTIONS, m_dwNumActions);
   msg->setFieldFromInt32Array(VID_RULE_ACTIONS, m_dwNumActions, m_pdwActionList);
   msg->setField(VID_NUM_EVENTS, m_dwNumEvents);
   msg->setFieldFromInt32Array(VID_RULE_EVENTS, m_dwNumEvents, m_pdwEventList);
   msg->setField(VID_NUM_SOURCES, m_dwNumSources);
   msg->setFieldFromInt32Array(VID_RULE_SOURCES, m_dwNumSources, m_pdwSourceList);
   msg->setField(VID_SCRIPT, CHECK_NULL_EX(m_pszScript));
   m_pstorageSetActions.fillMessage(msg, VID_NUM_SET_PSTORAGE, VID_PSTORAGE_SET_LIST_BASE);
   m_pstorageDeleteActions.fillMessage(msg, VID_PSTORAGE_DELETE_LIST_BASE, VID_NUM_DELETE_PSTORAGE);
}

/**
 * Check if given action is used within rule
 */
bool EPRule::isActionInUse(UINT32 dwActionId)
{
   UINT32 i;

   for(i = 0; i < m_dwNumActions; i++)
      if (m_pdwActionList[i] == dwActionId)
         return true;
   return false;
}

/**
 * Serialize rule to JSON
 */
json_t *EPRule::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "guid", m_guid.toJson());
   json_object_set_new(root, "flags", json_integer(m_dwFlags));

   json_t *sources = json_array();
   for(UINT32 i = 0; i < m_dwNumSources; i++)
      json_array_append_new(sources, json_integer(m_pdwSourceList[i]));
   json_object_set_new(root, "sources", sources);

   json_t *events = json_array();
   for(UINT32 i = 0; i < m_dwNumEvents; i++)
      json_array_append_new(events, json_integer(m_pdwEventList[i]));
   json_object_set_new(root, "events", events);

   json_t *actions = json_array();
   for(UINT32 i = 0; i < m_dwNumActions; i++)
      json_array_append_new(actions, json_integer(m_pdwActionList[i]));
   json_object_set_new(root, "actions", actions);

   json_object_set_new(root, "comments", json_string_t(m_pszComment));
   json_object_set_new(root, "script", json_string_t(m_pszScript));
   json_object_set_new(root, "alarmMessage", json_string_t(m_szAlarmMessage));
   json_object_set_new(root, "alarmSeverity", json_integer(m_iAlarmSeverity));
   json_object_set_new(root, "alarmKey", json_string_t(m_szAlarmKey));
   json_object_set_new(root, "alarmTimeout", json_integer(m_dwAlarmTimeout));
   json_object_set_new(root, "alarmTimeoutEvent", json_integer(m_dwAlarmTimeoutEvent));
   json_object_set_new(root, "categories", m_alarmCategoryList->toJson());
   json_object_set_new(root, "pstorageSetActions", m_pstorageSetActions.toJson());
   json_object_set_new(root, "pstorageDeleteActions", m_pstorageDeleteActions.toJson());

   return root;
}

/**
 * Event processing policy constructor
 */
EventPolicy::EventPolicy() : m_rules(128, 128, true)
{
   m_rwlock = RWLockCreate();
}

/**
 * Event processing policy destructor
 */
EventPolicy::~EventPolicy()
{
   RWLockDestroy(m_rwlock);
}

/**
 * Load event processing policy from database
 */
bool EventPolicy::loadFromDB()
{
   DB_RESULT hResult;
   bool success = false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   hResult = DBSelect(hdb, _T("SELECT rule_id,rule_guid,flags,comments,alarm_message,")
                           _T("alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event ")
                           _T("FROM event_policy ORDER BY rule_id"));
   if (hResult != NULL)
   {
      success = true;
      int count = DBGetNumRows(hResult);
      for(int i = 0; (i < count) && success; i++)
      {
         EPRule *rule = new EPRule(hResult, i);
         success = rule->loadFromDB(hdb);
         if (success)
            m_rules.add(rule);
         else
            delete rule;
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Save event processing policy to database
 */
bool EventPolicy::saveToDB() const
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = DBBegin(hdb);
   if (success)
   {
      success = DBQuery(hdb, _T("DELETE FROM event_policy")) &&
                DBQuery(hdb, _T("DELETE FROM policy_action_list")) &&
                DBQuery(hdb, _T("DELETE FROM policy_event_list")) &&
                DBQuery(hdb, _T("DELETE FROM policy_source_list")) &&
                DBQuery(hdb, _T("DELETE FROM policy_pstorage_actions")) &&
                DBQuery(hdb, _T("DELETE FROM alarm_category_map"));

      if (success)
      {
         readLock();
         for(int i = 0; (i < m_rules.size()) && success; i++)
            success = m_rules.get(i)->saveToDB(hdb);
         unlock();
      }

      if (success)
         DBCommit(hdb);
      else
         DBRollback(hdb);
   }
	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Pass event through policy
 */
void EventPolicy::processEvent(Event *pEvent)
{
	DbgPrintf(7, _T("EPP: processing event ") UINT64_FMT, pEvent->getId());
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
      if (m_rules.get(i)->processEvent(pEvent))
		{
			DbgPrintf(7, _T("EPP: got \"stop processing\" flag for event ") UINT64_FMT _T(" at rule %d"), pEvent->getId(), i + 1);
         break;   // EPRule::ProcessEvent() return TRUE if we should stop processing this event
		}
   unlock();
}

/**
 * Send event policy to client
 */
void EventPolicy::sendToClient(ClientSession *pSession, UINT32 dwRqId) const
{
   NXCPMessage msg;

   readLock();
   msg.setCode(CMD_EPP_RECORD);
   msg.setId(dwRqId);
   for(int i = 0; i < m_rules.size(); i++)
   {
      m_rules.get(i)->createMessage(&msg);
      pSession->sendMessage(&msg);
      msg.deleteAllFields();
   }
   unlock();
}

/**
 * Replace policy with new one
 */
void EventPolicy::replacePolicy(UINT32 dwNumRules, EPRule **ppRuleList)
{
   writeLock();
   m_rules.clear();
   if (ppRuleList != NULL)
   {
      for(int i = 0; i < (int)dwNumRules; i++)
      {
         EPRule *r = ppRuleList[i];
         r->setId(i);
         m_rules.add(r);
      }
   }
   unlock();
}

/**
 * Check if given action is used in policy
 */
bool EventPolicy::isActionInUse(UINT32 actionId) const
{
   bool bResult = false;

   readLock();

   for(int i = 0; i < m_rules.size(); i++)
      if (m_rules.get(i)->isActionInUse(actionId))
      {
         bResult = true;
         break;
      }

   unlock();
   return bResult;
}

/**
 * Check if given category is used in policy
 */
bool EventPolicy::isCategoryInUse(UINT32 categoryId) const
{
   bool bResult = false;

   readLock();

   for(int i = 0; i < m_rules.size(); i++)
      if (m_rules.get(i)->isCategoryInUse(categoryId))
      {
         bResult = true;
         break;
      }

   unlock();
   return bResult;
}

/**
 * Export rule
 */
void EventPolicy::exportRule(String& str, const uuid& guid) const
{
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      if (guid.equals(m_rules.get(i)->getGuid()))
      {
         m_rules.get(i)->createNXMPRecord(str);
         break;
      }
   }
   unlock();
}

/**
 * Import rule
 */
void EventPolicy::importRule(EPRule *rule)
{
   writeLock();

   // Find rule with same GUID and replace it if found
   bool newRule = true;
   for(int i = 0; i < m_rules.size(); i++)
   {
      if (rule->getGuid().equals(m_rules.get(i)->getGuid()))
      {
         rule->setId(i);
         m_rules.set(i, rule);
         newRule = false;
         break;
      }
   }

   // Add new rule at the end
   if (newRule)
   {
      rule->setId(m_rules.size());
      m_rules.add(rule);
   }

   unlock();
}

/**
 * Create JSON representation
 */
json_t *EventPolicy::toJson() const
{
   json_t *root = json_object();
   json_t *rules = json_array();
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      json_array_append_new(rules, m_rules.get(i)->toJson());
   }
   unlock();
   json_object_set_new(root, "rules", rules);
   return root;
}
