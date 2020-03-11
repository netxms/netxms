/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

#define DEBUG_TAG _T("event.policy")

/**
 * Default event policy rule constructor
 */
EPRule::EPRule(UINT32 id) : m_actions(0, 16, Ownership::True)
{
   m_id = id;
   m_guid = uuid::generate();
   m_flags = 0;
   m_comments = NULL;
   m_alarmSeverity = 0;
   m_alarmKey = NULL;
   m_alarmMessage = NULL;
   m_alarmImpact = NULL;
   m_rcaScriptName = NULL;
   m_scriptSource = NULL;
   m_script = NULL;
	m_alarmTimeout = 0;
	m_alarmTimeoutEvent = EVENT_ALARM_TIMEOUT;
}

/**
 * Create rule from config entry
 */
EPRule::EPRule(ConfigEntry *config) : m_actions(0, 16, Ownership::True)
{
   m_id = 0;
   m_guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (m_guid.isNull())
      m_guid = uuid::generate(); // generate random GUID if rule was imported without GUID
   m_flags = config->getSubEntryValueAsUInt(_T("flags"));

	ConfigEntry *eventsRoot = config->findEntry(_T("events"));
   if (eventsRoot != NULL)
   {
		ObjectArray<ConfigEntry> *events = eventsRoot->getSubEntries(_T("event#*"));
      for(int i = 0; i < events->size(); i++)
      {
         shared_ptr<EventTemplate> e = FindEventTemplateByName(events->get(i)->getSubEntryValue(_T("name"), 0, _T("<unknown>")));
         if (e != NULL)
         {
            m_events.add(e->getCode());
         }
      }
      delete events;
   }

   m_comments = _tcsdup(config->getSubEntryValue(_T("comments"), 0, _T("")));
   m_alarmSeverity = config->getSubEntryValueAsInt(_T("alarmSeverity"));
	m_alarmTimeout = config->getSubEntryValueAsUInt(_T("alarmTimeout"));
	m_alarmTimeoutEvent = config->getSubEntryValueAsUInt(_T("alarmTimeoutEvent"), 0, EVENT_ALARM_TIMEOUT);
   m_alarmKey = MemCopyString(config->getSubEntryValue(_T("alarmKey")));
   m_alarmMessage = MemCopyString(config->getSubEntryValue(_T("alarmMessage")));
   m_alarmImpact = MemCopyString(config->getSubEntryValue(_T("alarmImpact")));
   m_rcaScriptName = MemCopyString(config->getSubEntryValue(_T("rootCauseAnalysisScript")));

   ConfigEntry *pStorageEntry = config->findEntry(_T("pStorageActions"));
   if (pStorageEntry != NULL)
   {
      ObjectArray<ConfigEntry> *tmp = pStorageEntry->getSubEntries(_T("set#*"));
      for(int i = 0; i < tmp->size(); i++)
      {
         m_pstorageSetActions.set(tmp->get(i)->getAttribute(_T("key")), tmp->get(i)->getValue());
      }
      delete tmp;

      tmp = pStorageEntry->getSubEntries(_T("delete#*"));
      for(int i = 0; i < tmp->size(); i++)
      {
         m_pstorageDeleteActions.add(tmp->get(i)->getAttribute(_T("key")));
      }
      delete tmp;
   }

   ConfigEntry *alarmCategoriesEntry = config->findEntry(_T("alarmCategories"));
   if(alarmCategoriesEntry != NULL)
   {
      ObjectArray<ConfigEntry> *categories = alarmCategoriesEntry->getSubEntries(_T("category#*"));
      if (categories != NULL)
      {
         for (int i = 0; i < categories->size(); i++)
         {
            const TCHAR *name = categories->get(i)->getAttribute(_T("name"));
            const TCHAR *description = categories->get(i)->getValue();

            if ((name != NULL) && (*name != 0))
            {
               UINT32 id = GetAndUpdateAlarmCategoryByName(name, description);
               if(id > 0)
               {
                  m_alarmCategoryList.add(id);
               }
               else
               {
                  m_alarmCategoryList.add(CreateNewAlarmCategoryFromImport(name, CHECK_NULL_EX(description)));
               }
            }
         }
         delete categories;
      }
   }

   m_scriptSource = _tcsdup(config->getSubEntryValue(_T("script"), 0, _T("")));
   if ((m_scriptSource != NULL) && (*m_scriptSource != 0))
   {
      TCHAR szError[256];

      m_script = NXSLCompileAndCreateVM(m_scriptSource, szError, 256, new NXSL_ServerEnv);
      if (m_script != NULL)
      {
      	m_script->setGlobalVariable("CUSTOM_MESSAGE", m_script->createValue(_T("")));
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for event processing policy rule #%u (%s)"), m_id + 1, szError);
      }
   }
   else
   {
      m_script = NULL;
   }

   ConfigEntry *actionsRoot = config->findEntry(_T("actions"));
   if (actionsRoot != NULL)
   {
      ObjectArray<ConfigEntry> *actions = actionsRoot->getSubEntries(_T("action#*"));
      for(int i = 0; i < actions->size(); i++)
      {
         uuid guid = actions->get(i)->getSubEntryValueAsUUID(_T("guid"));
         UINT32 timerDelay = actions->get(i)->getSubEntryValueAsUInt(_T("timerDelay"));
         const TCHAR *timerKey = actions->get(i)->getSubEntryValue(_T("timerKey"));
         if (!guid.isNull())
         {
            UINT32 actionId = FindActionByGUID(guid);
            if (actionId != 0)
               m_actions.add(new ActionExecutionConfiguration(actionId, timerDelay, MemCopyString(timerKey)));
         }
         else
         {
            UINT32 actionId = actions->get(i)->getId();
            if (IsValidActionId(actionId))
               m_actions.add(new ActionExecutionConfiguration(actionId, timerDelay, MemCopyString(timerKey)));
         }
      }
      delete actions;
   }

   ConfigEntry *timerCancellationsRoot = config->findEntry(_T("timerCancellations"));
   if (timerCancellationsRoot != NULL)
   {
      ConfigEntry *keys = timerCancellationsRoot->findEntry(_T("timerKey"));
      if (keys != NULL)
      {
         for(int i = 0; i < keys->getValueCount(); i++)
         {
            const TCHAR *v = keys->getValue(i);
            if ((v != NULL) && (*v != 0))
               m_timerCancellations.add(v);
         }
      }
   }
}

/**
 * Construct event policy rule from database record
 * Assuming the following field order:
 * rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,
 * alarm_timeout,alarm_timeout_event,rca_script_name,alarm_impact
 */
EPRule::EPRule(DB_RESULT hResult, int row) : m_actions(0, 16, Ownership::True)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_guid = DBGetFieldGUID(hResult, row, 1);
   m_flags = DBGetFieldULong(hResult, row, 2);
   m_comments = DBGetField(hResult, row, 3, NULL, 0);
   m_alarmMessage = DBGetField(hResult, row, 4, NULL, 0);
   m_alarmSeverity = DBGetFieldLong(hResult, row, 5);
   m_alarmKey = DBGetField(hResult, row, 6, NULL, 0);
   m_scriptSource = DBGetField(hResult, row, 7, NULL, 0);
   if ((m_scriptSource != NULL) && (*m_scriptSource != 0))
   {
      TCHAR szError[256];

      m_script = NXSLCompileAndCreateVM(m_scriptSource, szError, 256, new NXSL_ServerEnv);
      if (m_script != NULL)
      {
      	m_script->setGlobalVariable("CUSTOM_MESSAGE", m_script->createValue(_T("")));
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for event processing policy rule #%u (%s)"), m_id + 1, szError);
      }
   }
   else
   {
      m_script = NULL;
   }
	m_alarmTimeout = DBGetFieldULong(hResult, row, 8);
	m_alarmTimeoutEvent = DBGetFieldULong(hResult, row, 9);
	m_rcaScriptName = DBGetField(hResult, row, 10, NULL, 0);
   m_alarmImpact = DBGetField(hResult, row, 11, NULL, 0);
}

/**
 * Construct event policy rule from NXCP message
 */
EPRule::EPRule(NXCPMessage *msg) : m_actions(0, 16, Ownership::True)
{
   m_flags = msg->getFieldAsUInt32(VID_FLAGS);
   m_id = msg->getFieldAsUInt32(VID_RULE_ID);
   m_guid = msg->getFieldAsGUID(VID_GUID);
   m_comments = msg->getFieldAsString(VID_COMMENTS);

   if (msg->isFieldExist(VID_RULE_ACTIONS))
   {
      IntegerArray<UINT32> actions;
      msg->getFieldAsInt32Array(VID_RULE_ACTIONS, &actions);
      for(int i = 0; i < actions.size(); i++)
      {
         m_actions.add(new ActionExecutionConfiguration(actions.get(i), 0, NULL));
      }
   }
   else
   {
      int count = msg->getFieldAsInt32(VID_NUM_ACTIONS);
      UINT32 fieldId = VID_ACTION_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         UINT32 actionId = msg->getFieldAsUInt32(fieldId++);
         UINT32 timerDelay = msg->getFieldAsUInt32(fieldId++);
         TCHAR *timerKey = msg->getFieldAsString(fieldId++);
         fieldId += 7;
         m_actions.add(new ActionExecutionConfiguration(actionId, timerDelay, timerKey));
      }
   }
   if (msg->isFieldExist(VID_TIMER_COUNT))
   {
      StringList list(msg, VID_TIMER_LIST_BASE, VID_TIMER_COUNT);
      m_timerCancellations.addAll(&list);
   }

   msg->getFieldAsInt32Array(VID_RULE_EVENTS, &m_events);
   msg->getFieldAsInt32Array(VID_RULE_SOURCES, &m_sources);

   m_alarmKey = msg->getFieldAsString(VID_ALARM_KEY);
   m_alarmMessage = msg->getFieldAsString(VID_ALARM_MESSAGE);
   m_alarmImpact = msg->getFieldAsString(VID_IMPACT);
   m_alarmSeverity = msg->getFieldAsUInt16(VID_ALARM_SEVERITY);
	m_alarmTimeout = msg->getFieldAsUInt32(VID_ALARM_TIMEOUT);
	m_alarmTimeoutEvent = msg->getFieldAsUInt32(VID_ALARM_TIMEOUT_EVENT);
	m_rcaScriptName = msg->getFieldAsString(VID_RCA_SCRIPT_NAME);

   msg->getFieldAsInt32Array(VID_ALARM_CATEGORY_ID, &m_alarmCategoryList);

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

   m_scriptSource = msg->getFieldAsString(VID_SCRIPT);
   if ((m_scriptSource != NULL) && (*m_scriptSource != 0))
   {
      TCHAR szError[256];

      m_script = NXSLCompileAndCreateVM(m_scriptSource, szError, 256, new NXSL_ServerEnv);
      if (m_script != NULL)
      {
      	m_script->setGlobalVariable("CUSTOM_MESSAGE", m_script->createValue(_T("")));
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for event processing policy rule #%u (%s)"), m_id + 1, szError);
      }
   }
   else
   {
      m_script = NULL;
   }
}

/**
 * Event policy rule destructor
 */
EPRule::~EPRule()
{
   MemFree(m_alarmMessage);
   MemFree(m_alarmImpact);
   MemFree(m_alarmKey);
   MemFree(m_rcaScriptName);
   MemFree(m_comments);
   MemFree(m_scriptSource);
   delete m_script;
}

/**
 * Create rule ordering entry
 */
void EPRule::createOrderingExportRecord(StringBuffer &xml) const
{
   xml.append(_T("\t\t<rule id=\""));
   xml.append(m_id + 1);
   xml.append(_T("\">"));
   xml.append(m_guid.toString());
   xml.append(_T("</rule>\n"));
}

/**
 * Create management pack record
 */
void EPRule::createExportRecord(StringBuffer &xml) const
{
   xml.append(_T("\t\t<rule id=\""));
   xml.append(m_id + 1);
   xml.append(_T("\">\n\t\t\t<guid>"));
   xml.append(m_guid.toString());
   xml.append(_T("</guid>\n\t\t\t<flags>"));
   xml.append(m_flags);
   xml.append(_T("</flags>\n\t\t\t<alarmMessage>"));
   xml.append(EscapeStringForXML2(m_alarmMessage));
   xml.append(_T("</alarmMessage>\n\t\t\t<alarmImpact>"));
   xml.append(EscapeStringForXML2(m_alarmImpact));
   xml.append(_T("</alarmImpact>\n\t\t\t<alarmKey>"));
   xml.append(EscapeStringForXML2(m_alarmKey));
   xml.append(_T("</alarmKey>\n\t\t\t<rootCauseAnalysisScript>"));
   xml.append(EscapeStringForXML2(m_rcaScriptName));
   xml.append(_T("</rootCauseAnalysisScript>\n\t\t\t<alarmSeverity>"));
   xml.append(m_alarmSeverity);
   xml.append(_T("</alarmSeverity>\n\t\t\t<alarmTimeout>"));
   xml.append(m_alarmTimeout);
   xml.append(_T("</alarmTimeout>\n\t\t\t<alarmTimeoutEvent>"));
   xml.append(m_alarmTimeoutEvent);
   xml.append(_T("</alarmTimeoutEvent>\n\t\t\t<script>"));
   xml.append(EscapeStringForXML2(m_scriptSource));
   xml.append(_T("</script>\n\t\t\t<comments>"));
   xml.append(EscapeStringForXML2(m_comments));
   xml.append(_T("</comments>\n\t\t\t<sources>\n"));

   for(int i = 0; i < m_sources.size(); i++)
   {
      NetObj *object = FindObjectById(m_sources.get(i));
      if (object != NULL)
      {
         TCHAR guidText[128];
         xml.appendFormattedString(_T("\t\t\t\t<source id=\"%d\">\n")
                                _T("\t\t\t\t\t<name>%s</name>\n")
                                _T("\t\t\t\t\t<guid>%s</guid>\n")
                                _T("\t\t\t\t\t<class>%d</class>\n")
                                _T("\t\t\t\t</source>\n"),
                                object->getId(),
                                (const TCHAR *)EscapeStringForXML2(object->getName()),
                                object->getGuid().toString(guidText), object->getObjectClass());
      }
   }

   xml += _T("\t\t\t</sources>\n\t\t\t<events>\n");

   for(int i = 0; i < m_events.size(); i++)
   {
      TCHAR eventName[MAX_EVENT_NAME];
      EventNameFromCode(m_events.get(i), eventName);
      xml.appendFormattedString(_T("\t\t\t\t<event id=\"%d\">\n")
                             _T("\t\t\t\t\t<name>%s</name>\n")
                             _T("\t\t\t\t</event>\n"),
                             m_events.get(i), (const TCHAR *)EscapeStringForXML2(eventName));
   }

   xml.append(_T("\t\t\t</events>\n\t\t\t<actions>\n"));
   for(int i = 0; i < m_actions.size(); i++)
   {
      xml.append(_T("\t\t\t\t<action id=\""));
      const ActionExecutionConfiguration *a = m_actions.get(i);
      xml.append(a->actionId);
      xml.append(_T("\">\n\t\t\t\t\t<guid>"));
      xml.append(GetActionGUID(a->actionId));
      xml.append(_T("</guid>\n\t\t\t\t\t<timerDelay>"));
      xml.append(a->timerDelay);
      xml.append(_T("</timerDelay>\n\t\t\t\t\t<timerKey>"));
      xml.append(a->timerKey);
      xml.append(_T("</timerKey>\n\t\t\t\t</action>\n"));
   }

   xml.append(_T("\t\t\t</actions>\n\t\t\t<timerCancellations>\n"));
   for(int i = 0; i < m_timerCancellations.size(); i++)
   {
      xml.append(_T("\t\t\t\t<timerKey>"));
      xml.append(m_timerCancellations.get(i));
      xml.append(_T("</timerKey>\n"));
   }

   xml.append(_T("\t\t\t</timerCancellations>\n\t\t\t<pStorageActions>\n"));
   StructArray<KeyValuePair<TCHAR>> *arr = m_pstorageSetActions.toArray();
   for(int i = 0; i < arr->size(); i++)
   {
      xml.appendFormattedString(_T("\t\t\t\t<set id=\"%d\" key=\"%s\">%s</set>\n"), i+1, arr->get(i)->key, arr->get(i)->value);
   }
   delete arr;
   for(int i = 0; i < m_pstorageDeleteActions.size(); i++)
   {
      xml.appendFormattedString(_T("\t\t\t\t<delete id=\"%d\" key=\"%s\"/>\n"), i+1, m_pstorageDeleteActions.get(i));
   }
   xml += _T("\t\t\t</pStorageActions>\n\t\t\t<alarmCategories>\n");
   for(int i = 0; i < m_alarmCategoryList.size(); i++)
   {
      AlarmCategory *category = GetAlarmCategory(m_alarmCategoryList.get(i));
      xml.appendFormattedString(_T("\t\t\t\t<category id=\"%d\" name=\"%s\">"), category->getId(), category->getName());
      xml.append(category->getDescription());
      xml.append(_T("</category>\n"));
      delete category;
   }
   xml.append(_T("\t\t\t</alarmCategories>\n\t\t</rule>\n"));
}

/**
 * Check if source object's id match to the rule
 */
bool EPRule::matchSource(UINT32 objectId)
{
   if (m_sources.isEmpty())
      return (m_flags & RF_NEGATED_SOURCE) ? false : true;

   bool match = false;
   for(int i = 0; i < m_sources.size(); i++)
   {
      if (m_sources.get(i) == objectId)
      {
         match = true;
         break;
      }

      NetObj *object = FindObjectById(m_sources.get(i));
      if (object != NULL)
      {
         if (object->isChild(objectId))
         {
            match = true;
            break;
         }
      }
      else
      {
         nxlog_write(NXLOG_WARNING, _T("Invalid object identifier %u in event processing policy rule #%u"), m_sources.get(i), m_id + 1);
      }
   }
   return (m_flags & RF_NEGATED_SOURCE) ? !match : match;
}

/**
 * Check if event's id match to the rule
 */
bool EPRule::matchEvent(UINT32 eventCode)
{
   if (m_events.isEmpty())
      return (m_flags & RF_NEGATED_EVENTS) ? false : true;

   bool match = false;
   for(int i = 0; i < m_events.size(); i++)
   {
      if (m_events.get(i) == eventCode)
      {
         match = true;
         break;
      }
   }
   return (m_flags & RF_NEGATED_EVENTS) ? !match : match;
}

/**
 * Check if event's severity match to the rule
 */
bool EPRule::matchSeverity(UINT32 severity)
{
   static UINT32 severityFlag[] = { RF_SEVERITY_INFO, RF_SEVERITY_WARNING,
                                    RF_SEVERITY_MINOR, RF_SEVERITY_MAJOR,
                                    RF_SEVERITY_CRITICAL };
	return (severityFlag[severity] & m_flags) != 0;
}

/**
 * Check if event match to the script
 */
bool EPRule::matchScript(Event *pEvent)
{
   bool bRet = true;

   if (m_script == NULL)
      return true;

   SetupServerScriptVM(m_script, FindObjectById(pEvent->getSourceId()), shared_ptr<DCObjectInfo>());
   m_script->setGlobalVariable("$event", m_script->createValue(new NXSL_Object(m_script, &g_nxslEventClass, pEvent, true)));
   m_script->setGlobalVariable("CUSTOM_MESSAGE", m_script->createValue());
   m_script->setGlobalVariable("EVENT_CODE", m_script->createValue(pEvent->getCode()));
   m_script->setGlobalVariable("SEVERITY", m_script->createValue(pEvent->getSeverity()));
   m_script->setGlobalVariable("SEVERITY_TEXT", m_script->createValue(GetStatusAsText(pEvent->getSeverity(), true)));
   m_script->setGlobalVariable("OBJECT_ID", m_script->createValue(pEvent->getSourceId()));
   m_script->setGlobalVariable("EVENT_TEXT", m_script->createValue((TCHAR *)pEvent->getMessage()));

   // Pass event's parameters as arguments and
   // other information as variables
   NXSL_Value **ppValueList = (NXSL_Value **)malloc(sizeof(NXSL_Value *) * pEvent->getParametersCount());
   memset(ppValueList, 0, sizeof(NXSL_Value *) * pEvent->getParametersCount());
   for(int i = 0; i < pEvent->getParametersCount(); i++)
      ppValueList[i] = m_script->createValue(pEvent->getParameter(i));

   // Run script
   NXSL_VariableSystem *globals = NULL;
   if (m_script->run(pEvent->getParametersCount(), ppValueList, &globals))
   {
      NXSL_Value *value = m_script->getResult();
      if (value != NULL)
      {
         bRet = value->getValueAsBoolean();
         if (bRet)
         {
         	NXSL_Variable *var = globals->find("CUSTOM_MESSAGE");
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
      PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, m_script->getErrorText(), 0);
      nxlog_write(NXLOG_ERROR, _T("Failed to execute evaluation script for event processing policy rule #%u (%s)"), m_id + 1, m_script->getErrorText());
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
   String psValue = pEvent->expandText(key);
   String psKey = pEvent->expandText((TCHAR *)value);
   SetPersistentStorageValue(psValue, psKey);
   return _CONTINUE;
}

/**
 * Check if event match to rule and perform required actions if yes
 * Method will return TRUE if event matched and RF_STOP_PROCESSING flag is set
 */
bool EPRule::processEvent(Event *event)
{
   if (m_flags & RF_DISABLED)
      return false;

   // Check if event match
   if (!matchSource(event->getSourceId()) || !matchEvent(event->getCode()) ||
       !matchSeverity(event->getSeverity()) || !matchScript(event))
      return false;

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Event ") UINT64_FMT _T(" match EPP rule %d"), event->getId(), (int)m_id + 1);

   // Generate alarm if requested
   UINT32 alarmId = 0;
   if (m_flags & RF_GENERATE_ALARM)
      alarmId = generateAlarm(event);

   // Execute actions
   if (!m_actions.isEmpty())
   {
      Alarm *alarm = FindAlarmById(alarmId);
      for(int i = 0; i < m_actions.size(); i++)
      {
         const ActionExecutionConfiguration *a = m_actions.get(i);
         if (a->timerDelay == 0)
         {
            ExecuteAction(a->actionId, event, alarm);
         }
         else
         {
            TCHAR parameters[64], comments[256];
            _sntprintf(parameters, 64, _T("action=%u;event=") UINT64_FMT _T(";alarm=%u"), a->actionId, event->getId(), (alarm != NULL) ? alarm->getAlarmId() : 0);
            _sntprintf(comments, 256, _T("Delayed action execution for event %s"), event->getName());
            String key = ((a->timerKey != NULL) && (*a->timerKey != 0)) ? event->expandText(a->timerKey, alarm) : String();
            AddOneTimeScheduledTask(_T("Execute.Action"), time(NULL) + a->timerDelay, parameters,
                     new ActionExecutionTransientData(event, alarm), 0, event->getSourceId(), SYSTEM_ACCESS_FULL,
                     comments, key.isEmpty() ? nullptr : key.cstr(), true);
         }
      }
      delete alarm;
   }

   // Cancel action timers
   if (!m_timerCancellations.isEmpty())
   {
      Alarm *alarm = FindAlarmById(alarmId);
      for(int i = 0; i < m_timerCancellations.size(); i++)
      {
         String key = event->expandText(m_timerCancellations.get(i), alarm);
         if (!key.isEmpty())
         {
            if (DeleteScheduledTaskByKey(key))
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Delayed action execution with key \"%s\" cancelled"), (const TCHAR *)key);
            }
         }
      }
      delete alarm;
   }

   // Update persistent storage if needed
   if (m_pstorageSetActions.size() > 0)
      m_pstorageSetActions.forEach(ExecutePstorageSetAction, event);
   for(int i = 0; i < m_pstorageDeleteActions.size(); i++)
   {
      String key = event->expandText(m_pstorageDeleteActions.get(i));
      if (!key.isEmpty())
         DeletePersistentStorageValue(key);
   }

   return (m_flags & RF_STOP_PROCESSING) ? true : false;
}

/**
 * Generate alarm from event
 */
UINT32 EPRule::generateAlarm(Event *event)
{
   UINT32 alarmId = 0;
   // Terminate alarms with key == our ack_key
	if ((m_alarmSeverity == SEVERITY_RESOLVE) || (m_alarmSeverity == SEVERITY_TERMINATE))
	{
		String key = event->expandText(m_alarmKey);
		if (!key.isEmpty())
		   ResolveAlarmByKey(key, (m_flags & RF_TERMINATE_BY_REGEXP) ? true : false, m_alarmSeverity == SEVERITY_TERMINATE, event);
	}
	else	// Generate new alarm
	{
	   UINT32 parentAlarmId = 0;
	   if ((m_rcaScriptName != NULL) && (m_rcaScriptName[0] != 0))
	   {
	      NetObj *object = FindObjectById(event->getSourceId());
	      NXSL_VM *vm = CreateServerScriptVM(m_rcaScriptName, object, nullptr);
	      if (vm != NULL)
	      {
	         vm->setGlobalVariable("$event", vm->createValue(new NXSL_Object(vm, &g_nxslEventClass, event, true)));
	         if (vm->run())
	         {
	            NXSL_Value *result = vm->getResult();
	            if (result->isObject(_T("Alarm")))
	            {
	               parentAlarmId = static_cast<Alarm*>(result->getValueAsObject()->getData())->getAlarmId();
	               nxlog_debug_tag(DEBUG_TAG, 5, _T("Root cause analysis script in rule %d has found parent alarm %u (%s)"),
	                        m_id + 1, parentAlarmId, static_cast<Alarm*>(result->getValueAsObject()->getData())->getMessage());
	            }
	         }
	         else
	         {
	            PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", m_rcaScriptName, vm->getErrorText(), 0);
	            nxlog_write(NXLOG_ERROR, _T("Failed to execute root cause analysis script for event processing policy rule #%u (%s)"), m_id + 1, vm->getErrorText());
	         }
	         delete vm;
	      }
	   }
	   alarmId = CreateNewAlarm(m_guid, CHECK_NULL_EX(m_alarmMessage), CHECK_NULL_EX(m_alarmKey), m_alarmImpact, ALARM_STATE_OUTSTANDING,
                     (m_alarmSeverity == SEVERITY_FROM_EVENT) ? event->getSeverity() : m_alarmSeverity,
                     m_alarmTimeout, m_alarmTimeoutEvent, parentAlarmId, m_rcaScriptName, event, 0, &m_alarmCategoryList,
                     ((m_flags & RF_CREATE_TICKET) != 0) ? true : false);
	}
	return alarmId;
}

/**
 * Load rule from database
 */
bool EPRule::loadFromDB(DB_HANDLE hdb)
{
   DB_RESULT hResult;
   TCHAR szQuery[256];
   bool bSuccess = true;

   // Load rule's sources
   _sntprintf(szQuery, 256, _T("SELECT object_id FROM policy_source_list WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
         m_sources.add(DBGetFieldULong(hResult, i, 0));
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
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
         m_events.add(DBGetFieldULong(hResult, i, 0));
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = false;
   }

   // Load rule's actions
   _sntprintf(szQuery, 256, _T("SELECT action_id,timer_delay,timer_key FROM policy_action_list WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 actionId = DBGetFieldULong(hResult, i, 0);
         UINT32 timerDelay = DBGetFieldULong(hResult, i, 1);
         TCHAR *timerKey = DBGetField(hResult, i, 2, NULL, 0);
         m_actions.add(new ActionExecutionConfiguration(actionId, timerDelay, timerKey));
      }
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = false;
   }

   // Load timer cancellations
   _sntprintf(szQuery, 256, _T("SELECT timer_key FROM policy_timer_cancellation_list WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         m_timerCancellations.addPreallocated(DBGetField(hResult, i, 0, NULL, 0));
      }
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
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, key, MAX_DB_STRING);
         if (DBGetFieldULong(hResult, i, 1) == PSTORAGE_SET)
         {
            m_pstorageSetActions.setPreallocated(_tcsdup(key), DBGetField(hResult, i, 2, NULL, 0));
         }
         if (DBGetFieldULong(hResult, i, 1) == PSTORAGE_DELETE)
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
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         m_alarmCategoryList.add(DBGetFieldULong(hResult, i, 0));
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
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC, 127);
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
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_impact,")
                                  _T("alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,rca_script_name) ")
                                  _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?)"));
   if (hStmt != NULL)
   {
      TCHAR guidText[128];
      DBBind(hStmt, 1, DB_CTYPE_INT32, m_id);
      DBBind(hStmt, 2, DB_CTYPE_STRING, m_guid.toString(guidText), DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_CTYPE_INT32, m_flags);
      DBBind(hStmt, 4, DB_CTYPE_STRING,  m_comments, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_CTYPE_STRING, m_alarmMessage, DB_BIND_STATIC, MAX_EVENT_MSG_LENGTH);
      DBBind(hStmt, 6, DB_CTYPE_STRING, m_alarmImpact, DB_BIND_STATIC, 1000);
      DBBind(hStmt, 7, DB_CTYPE_INT32, m_alarmSeverity);
      DBBind(hStmt, 8, DB_CTYPE_STRING, m_alarmKey, DB_BIND_STATIC, MAX_DB_STRING);
      DBBind(hStmt, 9, DB_CTYPE_STRING, m_scriptSource, DB_BIND_STATIC);
      DBBind(hStmt, 10, DB_CTYPE_INT32, m_alarmTimeout);
      DBBind(hStmt, 11, DB_CTYPE_INT32, m_alarmTimeoutEvent);
      DBBind(hStmt, 12, DB_CTYPE_STRING, m_rcaScriptName, DB_BIND_STATIC, MAX_DB_STRING);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   else
   {
      success = false;
   }

   // Actions
   if (success && !m_actions.isEmpty())
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO policy_action_list (rule_id,action_id,timer_delay,timer_key) VALUES (?,?,?,?)"), m_actions.size() > 1);
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(i = 0; i < m_actions.size() && success; i++)
         {
            const ActionExecutionConfiguration *a = m_actions.get(i);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, a->actionId);
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, a->timerDelay);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, a->timerKey, DB_BIND_STATIC, 127);
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Timer cancellations
   if (success && !m_timerCancellations.isEmpty())
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO policy_timer_cancellation_list (rule_id,timer_key) VALUES (?,?)"), m_timerCancellations.size() > 1);
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(i = 0; i < m_timerCancellations.size() && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_timerCancellations.get(i), DB_BIND_STATIC, 127);
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Events
   if (success && !m_events.isEmpty())
   {
      for(i = 0; i < m_events.size() && success; i++)
      {
         _sntprintf(pszQuery, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), m_id, m_events.get(i));
         success = DBQuery(hdb, pszQuery);
      }
   }

   // Sources
   if (success && !m_sources.isEmpty())
   {
      for(i = 0; i < m_sources.size() && success; i++)
      {
         _sntprintf(pszQuery, 1024, _T("INSERT INTO policy_source_list (rule_id,object_id) VALUES (%d,%d)"), m_id, m_sources.get(i));
         success = DBQuery(hdb, pszQuery);
      }
   }

	// Persistent storage actions
   if (success && !m_pstorageSetActions.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO policy_pstorage_actions (rule_id,action,ps_key,value) VALUES (?,1,?,?)"), m_pstorageSetActions.size() > 1);
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         success = _STOP != m_pstorageSetActions.forEach(SavePstorageSetActions, hStmt);
         DBFreeStatement(hStmt);
      }
   }

   if (success && !m_pstorageDeleteActions.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO policy_pstorage_actions (rule_id,action,ps_key) VALUES (?,2,?)"), m_pstorageDeleteActions.size() > 1);
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; i < m_pstorageDeleteActions.size() && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_pstorageDeleteActions.get(i), DB_BIND_STATIC, 127);
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
      }
   }

   // Alarm categories
   if (success && !m_alarmCategoryList.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO alarm_category_map (alarm_id,category_id) VALUES (?,?)"), m_alarmCategoryList.size() > 1);
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER && success, m_id);
         for(i = 0; (i < m_alarmCategoryList.size()) && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_alarmCategoryList.get(i));
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
      }
   }

   return success;
}

/**
 * Create NXCP message with rule's data
 */
void EPRule::createMessage(NXCPMessage *msg)
{
   msg->setField(VID_FLAGS, m_flags);
   msg->setField(VID_RULE_ID, m_id);
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_ALARM_SEVERITY, (WORD)m_alarmSeverity);
   msg->setField(VID_ALARM_KEY, m_alarmKey);
   msg->setField(VID_ALARM_MESSAGE, m_alarmMessage);
   msg->setField(VID_IMPACT, m_alarmImpact);
   msg->setField(VID_ALARM_TIMEOUT, m_alarmTimeout);
   msg->setField(VID_ALARM_TIMEOUT_EVENT, m_alarmTimeoutEvent);
   msg->setFieldFromInt32Array(VID_ALARM_CATEGORY_ID, &m_alarmCategoryList);
   msg->setField(VID_RCA_SCRIPT_NAME, m_rcaScriptName);
   msg->setField(VID_COMMENTS, CHECK_NULL_EX(m_comments));
   msg->setField(VID_NUM_ACTIONS, (UINT32)m_actions.size());
   UINT32 fieldId = VID_ACTION_LIST_BASE;
   for(int i = 0; i < m_actions.size(); i++)
   {
      const ActionExecutionConfiguration *a = m_actions.get(i);
      msg->setField(fieldId++, a->actionId);
      msg->setField(fieldId++, a->timerDelay);
      msg->setField(fieldId++, a->timerKey);
      fieldId += 7;
   }
   m_timerCancellations.fillMessage(msg, VID_TIMER_LIST_BASE, VID_TIMER_COUNT);
   msg->setField(VID_NUM_EVENTS, (UINT32)m_events.size());
   msg->setFieldFromInt32Array(VID_RULE_EVENTS, &m_events);
   msg->setField(VID_NUM_SOURCES, (UINT32)m_sources.size());
   msg->setFieldFromInt32Array(VID_RULE_SOURCES, &m_sources);
   msg->setField(VID_SCRIPT, CHECK_NULL_EX(m_scriptSource));
   m_pstorageSetActions.fillMessage(msg, VID_NUM_SET_PSTORAGE, VID_PSTORAGE_SET_LIST_BASE);
   m_pstorageDeleteActions.fillMessage(msg, VID_PSTORAGE_DELETE_LIST_BASE, VID_NUM_DELETE_PSTORAGE);
}

/**
 * Serialize rule to JSON
 */
json_t *EPRule::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "guid", m_guid.toJson());
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "sources", m_sources.toJson());
   json_object_set_new(root, "events", m_events.toJson());
   json_t *actions = json_array();
   for(int i = 0; i < m_actions.size(); i++)
   {
      const ActionExecutionConfiguration *d = m_actions.get(i);
      json_t *action = json_object();
      json_object_set_new(action, "id", json_integer(d->actionId));
      json_object_set_new(action, "timerDelay", json_integer(d->timerDelay));
      json_object_set_new(action, "timerKey", json_string_t(d->timerKey));
      json_array_append_new(actions, action);
   }
   json_object_set_new(root, "actions", actions);
   json_t *timerCancellations = json_array();
   for(int i = 0; i < m_timerCancellations.size(); i++)
   {
      json_array_append_new(timerCancellations, json_string_t(m_timerCancellations.get(i)));
   }
   json_object_set_new(root, "timerCancellations", timerCancellations);
   json_object_set_new(root, "comments", json_string_t(m_comments));
   json_object_set_new(root, "script", json_string_t(m_scriptSource));
   json_object_set_new(root, "alarmMessage", json_string_t(m_alarmMessage));
   json_object_set_new(root, "alarmImpact", json_string_t(m_alarmImpact));
   json_object_set_new(root, "alarmSeverity", json_integer(m_alarmSeverity));
   json_object_set_new(root, "alarmKey", json_string_t(m_alarmKey));
   json_object_set_new(root, "alarmTimeout", json_integer(m_alarmTimeout));
   json_object_set_new(root, "alarmTimeoutEvent", json_integer(m_alarmTimeoutEvent));
   json_object_set_new(root, "rcaScriptName", json_string_t(m_rcaScriptName));
   json_object_set_new(root, "categories", m_alarmCategoryList.toJson());
   json_object_set_new(root, "pstorageSetActions", m_pstorageSetActions.toJson());
   json_object_set_new(root, "pstorageDeleteActions", m_pstorageDeleteActions.toJson());

   return root;
}

/**
 * Check if given action is in use by this rule
 */
bool EPRule::isActionInUse(UINT32 actionId) const
{
   for(int i = 0; i < m_actions.size(); i++)
      if (m_actions.get(i)->actionId == actionId)
         return true;
   return false;
}

/**
 * Event processing policy constructor
 */
EventPolicy::EventPolicy() : m_rules(128, 128, Ownership::True)
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
                           _T("alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,")
                           _T("rca_script_name,alarm_impact FROM event_policy ORDER BY rule_id"));
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
                DBQuery(hdb, _T("DELETE FROM policy_timer_cancellation_list")) &&
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
	nxlog_debug_tag(DEBUG_TAG, 7, _T("EPP: processing event ") UINT64_FMT, pEvent->getId());
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
      if (m_rules.get(i)->processEvent(pEvent))
		{
			nxlog_debug_tag(DEBUG_TAG, 7, _T("EPP: got \"stop processing\" flag for event ") UINT64_FMT _T(" at rule %d"), pEvent->getId(), i + 1);
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
void EventPolicy::exportRule(StringBuffer& xml, const uuid& guid) const
{
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      if (guid.equals(m_rules.get(i)->getGuid()))
      {
         m_rules.get(i)->createExportRecord(xml);
         break;
      }
   }
   unlock();
}

/**
 * Export rules ordering
 */
void EventPolicy::exportRuleOrgering(StringBuffer& xml) const
{
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      m_rules.get(i)->createOrderingExportRecord(xml);
   }
   unlock();
}

/**
 * Finds rule index by guid and adds index shift if found
 */
int EventPolicy::findRuleIndexByGuid(const uuid& guid, int shift) const
{
   for (int i = 0; i < m_rules.size(); i++)
   {
      if (guid.equals(m_rules.get(i)->getGuid()))
      {
         return i + shift;
      }
   }
   return -1;
}

/**
 * Import rule
 */
void EventPolicy::importRule(EPRule *rule, bool overwrite, ObjectArray<uuid> *ruleOrdering)
{
   writeLock();

   // Find rule with same GUID and replace it if found
   int ruleIndex = findRuleIndexByGuid(rule->getGuid());
   if (ruleIndex != -1)
   {
      if (overwrite)
      {
         rule->setId(ruleIndex);
         m_rules.set(ruleIndex, rule);
      }
      else
      {
         delete rule;
      }
   }
   else //insert new rule
   {
      bool foundPreviousIndex = true;
      if(ruleOrdering != NULL)
      {
         int newRulePrevIndex = -1;
         for (int i = 0; i < ruleOrdering->size(); i++)
         {
            if(ruleOrdering->get(i)->equals(rule->getGuid()))
            {
               newRulePrevIndex = i;
               break;
            }
         }

         if(newRulePrevIndex != -1)
         {
            //find rule before this rule
            if (newRulePrevIndex > 0)
               ruleIndex = findRuleIndexByGuid(*ruleOrdering->get(newRulePrevIndex - 1), 1);

            //if rule after this rule if before not found
            if(ruleIndex == -1 && (newRulePrevIndex + 1) < ruleOrdering->size())
               ruleIndex = findRuleIndexByGuid(*ruleOrdering->get(newRulePrevIndex + 1));

            //check if any rule before this rule already exist if before not found
            for (int i = newRulePrevIndex - 2; ruleIndex == -1 && i >= 0; i--)
               ruleIndex = findRuleIndexByGuid(*ruleOrdering->get(i), 1);

            //check if any rule after this rule already exist if before not found
            for (int i = newRulePrevIndex + 2; ruleIndex == -1 && i < ruleOrdering->size(); i++)
               ruleIndex = findRuleIndexByGuid(*ruleOrdering->get(i));

         }
      }

      if(ruleIndex == -1) // Add new rule at the end
      {
         rule->setId(m_rules.size());
         m_rules.add(rule);
      }
      else
      {
         rule->setId(ruleIndex);
         m_rules.insert(ruleIndex, rule);
         for(int i = ruleIndex + 1; i < m_rules.size(); i++)
         {
            m_rules.get(i)->setId(i);
         }
      }
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
