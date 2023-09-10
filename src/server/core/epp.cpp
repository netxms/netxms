/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
#include <nxcore_ps.h>

#define DEBUG_TAG _T("event.policy")

/**
 * Default event policy rule constructor
 */
EPRule::EPRule(uint32_t id) : m_actions(0, 16, Ownership::True)
{
   m_id = id;
   m_guid = uuid::generate();
   m_flags = 0;
   m_comments = nullptr;
   m_alarmSeverity = 0;
   m_alarmKey = nullptr;
   m_alarmMessage = nullptr;
   m_alarmImpact = nullptr;
   m_rcaScriptName = nullptr;
   m_scriptSource = nullptr;
   m_script = nullptr;
	m_alarmTimeout = 0;
	m_alarmTimeoutEvent = EVENT_ALARM_TIMEOUT;
}

/**
 * Create rule from config entry
 */
EPRule::EPRule(const ConfigEntry& config) : m_actions(0, 16, Ownership::True)
{
   m_id = 0;
   m_guid = config.getSubEntryValueAsUUID(_T("guid"));
   if (m_guid.isNull())
      m_guid = uuid::generate(); // generate random GUID if rule was imported without GUID
   m_flags = config.getSubEntryValueAsUInt(_T("flags"));

	ConfigEntry *eventsRoot = config.findEntry(_T("events"));
   if (eventsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> events = eventsRoot->getSubEntries(_T("event#*"));
      for(int i = 0; i < events->size(); i++)
      {
         shared_ptr<EventTemplate> e = FindEventTemplateByName(events->get(i)->getSubEntryValue(_T("name"), 0, _T("<unknown>")));
         if (e != nullptr)
         {
            m_events.add(e->getCode());
         }
      }
   }

   m_comments = _tcsdup(config.getSubEntryValue(_T("comments"), 0, _T("")));
   m_alarmSeverity = config.getSubEntryValueAsInt(_T("alarmSeverity"));
	m_alarmTimeout = config.getSubEntryValueAsUInt(_T("alarmTimeout"));
	m_alarmTimeoutEvent = config.getSubEntryValueAsUInt(_T("alarmTimeoutEvent"), 0, EVENT_ALARM_TIMEOUT);
   m_alarmKey = MemCopyString(config.getSubEntryValue(_T("alarmKey")));
   m_alarmMessage = MemCopyString(config.getSubEntryValue(_T("alarmMessage")));
   m_alarmImpact = MemCopyString(config.getSubEntryValue(_T("alarmImpact")));
   m_rcaScriptName = MemCopyString(config.getSubEntryValue(_T("rootCauseAnalysisScript")));

   ConfigEntry *pStorageEntry = config.findEntry(_T("pStorageActions"));
   if (pStorageEntry != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> tmp = pStorageEntry->getSubEntries(_T("set#*"));
      for(int i = 0; i < tmp->size(); i++)
      {
         m_pstorageSetActions.set(tmp->get(i)->getAttribute(_T("key")), tmp->get(i)->getValue());
      }

      tmp = pStorageEntry->getSubEntries(_T("delete#*"));
      for(int i = 0; i < tmp->size(); i++)
      {
         m_pstorageDeleteActions.add(tmp->get(i)->getAttribute(_T("key")));
      }
   }

   ConfigEntry *alarmCategoriesEntry = config.findEntry(_T("alarmCategories"));
   if(alarmCategoriesEntry != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> categories = alarmCategoriesEntry->getSubEntries(_T("category#*"));
      if (categories != nullptr)
      {
         for (int i = 0; i < categories->size(); i++)
         {
            const TCHAR *name = categories->get(i)->getAttribute(_T("name"));
            const TCHAR *description = categories->get(i)->getValue();

            if ((name != nullptr) && (*name != 0))
            {
               uint32_t id = UpdateAlarmCategoryDescription(name, CHECK_NULL_EX(description));
               if (id > 0)
               {
                  m_alarmCategoryList.add(id);
               }
               else
               {
                  m_alarmCategoryList.add(CreateAlarmCategory(name, CHECK_NULL_EX(description)));
               }
            }
         }
      }
   }

   m_scriptSource = _tcsdup(config.getSubEntryValue(_T("script"), 0, _T("")));
   if ((m_scriptSource != nullptr) && (*m_scriptSource != 0))
   {
      TCHAR errorText[256];
      NXSL_ServerEnv env;
      m_script = NXSLCompile(m_scriptSource, errorText, 256, nullptr, &env);
      if (m_script == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for event processing policy rule #%u (%s)"), m_id + 1, errorText);
      }
   }
   else
   {
      m_script = nullptr;
   }

   ConfigEntry *actionsRoot = config.findEntry(_T("actions"));
   if (actionsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> actions = actionsRoot->getSubEntries(_T("action#*"));
      for(int i = 0; i < actions->size(); i++)
      {
         uuid guid = actions->get(i)->getSubEntryValueAsUUID(_T("guid"));
         const TCHAR *timerDelay = actions->get(i)->getSubEntryValue(_T("timerDelay"));
         const TCHAR *timerKey = actions->get(i)->getSubEntryValue(_T("timerKey"));
         const TCHAR *blockingTimerKey = actions->get(i)->getSubEntryValue(_T("blockingTimerKey"));
         const TCHAR *snoozeTime = actions->get(i)->getSubEntryValue(_T("snoozeTime"));
         if (!guid.isNull())
         {
            UINT32 actionId = FindActionByGUID(guid);
            if (actionId != 0)
               m_actions.add(new ActionExecutionConfiguration(actionId, MemCopyString(timerDelay), MemCopyString(snoozeTime), MemCopyString(timerKey), MemCopyString(blockingTimerKey)));
         }
         else
         {
            UINT32 actionId = actions->get(i)->getId();
            if (IsValidActionId(actionId))
               m_actions.add(new ActionExecutionConfiguration(actionId, MemCopyString(timerDelay), MemCopyString(snoozeTime), MemCopyString(timerKey), MemCopyString(blockingTimerKey)));
         }
      }
   }

   ConfigEntry *timerCancellationsRoot = config.findEntry(_T("timerCancellations"));
   if (timerCancellationsRoot != nullptr)
   {
      ConfigEntry *keys = timerCancellationsRoot->findEntry(_T("timerKey"));
      if (keys != nullptr)
      {
         for(int i = 0; i < keys->getValueCount(); i++)
         {
            const TCHAR *v = keys->getValue(i);
            if ((v != nullptr) && (*v != 0))
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
   m_comments = DBGetField(hResult, row, 3, nullptr, 0);
   m_alarmMessage = DBGetField(hResult, row, 4, nullptr, 0);
   m_alarmSeverity = DBGetFieldLong(hResult, row, 5);
   m_alarmKey = DBGetField(hResult, row, 6, nullptr, 0);
   m_scriptSource = DBGetField(hResult, row, 7, nullptr, 0);
   if ((m_scriptSource != nullptr) && (*m_scriptSource != 0))
   {
      TCHAR errorText[256];
      NXSL_ServerEnv env;
      m_script = NXSLCompile(m_scriptSource, errorText, 256, nullptr, &env);
      if (m_script == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for event processing policy rule #%u (%s)"), m_id + 1, errorText);
      }
   }
   else
   {
      m_script = nullptr;
   }
	m_alarmTimeout = DBGetFieldULong(hResult, row, 8);
	m_alarmTimeoutEvent = DBGetFieldULong(hResult, row, 9);
	m_rcaScriptName = DBGetField(hResult, row, 10, nullptr, 0);
   m_alarmImpact = DBGetField(hResult, row, 11, nullptr, 0);
}

/**
 * Construct event policy rule from NXCP message
 */
EPRule::EPRule(const NXCPMessage& msg) : m_actions(0, 16, Ownership::True)
{
   m_flags = msg.getFieldAsUInt32(VID_FLAGS);
   m_id = msg.getFieldAsUInt32(VID_RULE_ID);
   m_guid = msg.getFieldAsGUID(VID_GUID);
   m_comments = msg.getFieldAsString(VID_COMMENTS);

   if (msg.isFieldExist(VID_RULE_ACTIONS))
   {
      IntegerArray<uint32_t> actions;
      msg.getFieldAsInt32Array(VID_RULE_ACTIONS, &actions);
      for(int i = 0; i < actions.size(); i++)
      {
         m_actions.add(new ActionExecutionConfiguration(actions.get(i), nullptr, nullptr, nullptr, nullptr));
      }
   }
   else
   {
      int count = msg.getFieldAsInt32(VID_NUM_ACTIONS);
      uint32_t fieldId = VID_ACTION_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         uint32_t actionId = msg.getFieldAsUInt32(fieldId++);
         TCHAR *timerDelay = msg.getFieldAsString(fieldId++);
         TCHAR *timerKey = msg.getFieldAsString(fieldId++);
         TCHAR *blockingTimerKey = msg.getFieldAsString(fieldId++);
         TCHAR *snoozeTime = msg.getFieldAsString(fieldId++);
         fieldId += 5;
         m_actions.add(new ActionExecutionConfiguration(actionId, timerDelay, snoozeTime, timerKey, blockingTimerKey));
      }
   }
   if (msg.isFieldExist(VID_TIMER_COUNT))
   {
      StringList list(msg, VID_TIMER_LIST_BASE, VID_TIMER_COUNT);
      m_timerCancellations.addAll(&list);
   }

   msg.getFieldAsInt32Array(VID_RULE_EVENTS, &m_events);
   msg.getFieldAsInt32Array(VID_RULE_SOURCES, &m_sources);

   m_alarmKey = msg.getFieldAsString(VID_ALARM_KEY);
   m_alarmMessage = msg.getFieldAsString(VID_ALARM_MESSAGE);
   m_alarmImpact = msg.getFieldAsString(VID_IMPACT);
   m_alarmSeverity = msg.getFieldAsUInt16(VID_ALARM_SEVERITY);
	m_alarmTimeout = msg.getFieldAsUInt32(VID_ALARM_TIMEOUT);
	m_alarmTimeoutEvent = msg.getFieldAsUInt32(VID_ALARM_TIMEOUT_EVENT);
	m_rcaScriptName = msg.getFieldAsString(VID_RCA_SCRIPT_NAME);

   msg.getFieldAsInt32Array(VID_ALARM_CATEGORY_ID, &m_alarmCategoryList);

   int count = msg.getFieldAsInt32(VID_NUM_SET_PSTORAGE);
   int base = VID_PSTORAGE_SET_LIST_BASE;
   for(int i = 0; i < count; i++, base+=2)
   {
      m_pstorageSetActions.setPreallocated(msg.getFieldAsString(base), msg.getFieldAsString(base+1));
   }

   count = msg.getFieldAsInt32(VID_NUM_DELETE_PSTORAGE);
   base = VID_PSTORAGE_DELETE_LIST_BASE;
   for(int i = 0; i < count; i++, base++)
   {
      m_pstorageDeleteActions.addPreallocated(msg.getFieldAsString(base));
   }

   m_scriptSource = msg.getFieldAsString(VID_SCRIPT);
   if ((m_scriptSource != nullptr) && (*m_scriptSource != 0))
   {
      TCHAR errorText[256];
      NXSL_ServerEnv env;
      m_script = NXSLCompile(m_scriptSource, errorText, 256, nullptr, &env);
      if (m_script == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for event processing policy rule #%u (%s)"), m_id + 1, errorText);
      }
   }
   else
   {
      m_script = nullptr;
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
      shared_ptr<NetObj> object = FindObjectById(m_sources.get(i));
      if (object != nullptr)
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
      xml.append((const TCHAR *)EscapeStringForXML2(a->timerDelay));
      xml.append(_T("</timerDelay>\n\t\t\t\t\t<timerKey>"));
      xml.append((const TCHAR *)EscapeStringForXML2(a->timerKey));
      xml.append(_T("</timerKey>\n\t\t\t\t\t<blockingTimerKey>"));
      xml.append((const TCHAR *)EscapeStringForXML2(a->blockingTimerKey));
      xml.append(_T("</blockingTimerKey>\n\t\t\t\t\t<snoozeTime>"));
      xml.append((const TCHAR *)EscapeStringForXML2(a->snoozeTime));
      xml.append(_T("</snoozeTime>\n\t\t\t\t</action>\n"));
   }

   xml.append(_T("\t\t\t</actions>\n\t\t\t<timerCancellations>\n"));
   for(int i = 0; i < m_timerCancellations.size(); i++)
   {
      xml.append(_T("\t\t\t\t<timerKey>"));
      xml.append(m_timerCancellations.get(i));
      xml.append(_T("</timerKey>\n"));
   }

   xml.append(_T("\t\t\t</timerCancellations>\n\t\t\t<pStorageActions>\n"));
   int id = 0;
   for(KeyValuePair<const TCHAR> *action : m_pstorageSetActions)
   {
      xml.appendFormattedString(_T("\t\t\t\t<set id=\"%d\" key=\"%s\">%s</set>\n"), ++id, action->key, action->value);
   }
   for(int i = 0; i < m_pstorageDeleteActions.size(); i++)
   {
      xml.appendFormattedString(_T("\t\t\t\t<delete id=\"%d\" key=\"%s\"/>\n"), i + 1, m_pstorageDeleteActions.get(i));
   }

   xml.append(_T("\t\t\t</pStorageActions>\n\t\t\t<alarmCategories>\n"));
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
bool EPRule::matchSource(const shared_ptr<NetObj>& object) const
{
   if (m_sources.isEmpty())
      return (m_flags & RF_NEGATED_SOURCE) ? false : true;

   if (object == nullptr)
      return (m_flags & RF_NEGATED_SOURCE) ? true : false;

   uint32_t objectId = object->getId();
   bool match = false;
   for(int i = 0; i < m_sources.size(); i++)
   {
      uint32_t id = m_sources.get(i);
      if (id == objectId)
      {
         match = true;
         break;
      }

      if (object->isParent(id))
      {
         match = true;
         break;
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
bool EPRule::matchEvent(uint32_t eventCode) const
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
bool EPRule::matchSeverity(uint32_t severity) const
{
   static uint32_t severityFlag[] = { RF_SEVERITY_INFO, RF_SEVERITY_WARNING, RF_SEVERITY_MINOR, RF_SEVERITY_MAJOR, RF_SEVERITY_CRITICAL };
	return (severityFlag[severity] & m_flags) != 0;
}

/**
 * Check if event match to the script
 */
bool EPRule::matchScript(Event *event) const
{
   bool bRet = true;

   if (m_script == nullptr)
      return true;

   ScriptVMHandle vm = CreateServerScriptVM(m_script, FindObjectById(event->getSourceId()), shared_ptr<DCObjectInfo>());
   if (!vm.isValid())
   {
      if (vm.failureReason() != ScriptVMFailureReason::SCRIPT_IS_EMPTY)
      {
         ReportScriptError(SCRIPT_CONTEXT_EVENT_PROC, FindObjectById(event->getSourceId()).get(), 0, _T("Script load failed"), _T("EPP::%d"), m_id + 1);
         nxlog_write(NXLOG_ERROR, _T("Cannot create NXSL VM for evaluation script for event processing policy rule #%u"), m_id + 1);
      }
      return true;
   }

   vm->setGlobalVariable("$event", vm->createValue(vm->createObject(&g_nxslEventClass, event, true)));
   vm->setGlobalVariable("CUSTOM_MESSAGE", vm->createValue());
   vm->setGlobalVariable("EVENT_CODE", vm->createValue(event->getCode()));
   vm->setGlobalVariable("SEVERITY", vm->createValue(event->getSeverity()));
   vm->setGlobalVariable("SEVERITY_TEXT", vm->createValue(GetStatusAsText(event->getSeverity(), true)));
   vm->setGlobalVariable("OBJECT_ID", vm->createValue(event->getSourceId()));
   vm->setGlobalVariable("EVENT_TEXT", vm->createValue(event->getMessage()));

   // Pass event's parameters as arguments and
   // other information as variables
   ObjectRefArray<NXSL_Value> args(event->getParametersCount());
   for(int i = 0; i < event->getParametersCount(); i++)
      args.add(vm->createValue(event->getParameter(i)));

   // Run script
   NXSL_VariableSystem *globals = nullptr;
   if (vm->run(args, &globals))
   {
      NXSL_Value *value = vm->getResult();
      bRet = value->getValueAsBoolean();
      if (bRet)
      {
         NXSL_Variable *var = globals->find("CUSTOM_MESSAGE");
         if ((var != nullptr) && var->getValue()->isString())
         {
            // Update custom message in event
            event->setCustomMessage(CHECK_NULL_EX(var->getValue()->getValueAsCString()));
         }
      }
   }
   else
   {
      ReportScriptError(SCRIPT_CONTEXT_EVENT_PROC, FindObjectById(event->getSourceId()).get(), 0, vm->getErrorText(), _T("EPP::%d"), m_id + 1);
      nxlog_write(NXLOG_ERROR, _T("Failed to execute evaluation script for event processing policy rule #%u (%s)"), m_id + 1, vm->getErrorText());
   }
   delete globals;
   vm.destroy();

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
bool EPRule::processEvent(Event *event) const
{
   if (m_flags & RF_DISABLED)
      return false;

   if ((event->getRootId() != 0) && !(m_flags & RF_ACCEPT_CORRELATED))
      return false;

   if (!matchSeverity(event->getSeverity()) || !matchEvent(event->getCode()))
      return false;

   shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
   if (!matchSource(object) || !matchScript(event))
      return false;

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Event ") UINT64_FMT _T(" match EPP rule %d"), event->getId(), (int)m_id + 1);

   // Generate alarm if requested
   uint32_t alarmId = 0;
   if (m_flags & RF_GENERATE_ALARM)
      alarmId = generateAlarm(event);

   // Execute actions
   if (!m_actions.isEmpty())
   {
      Alarm *alarm = FindAlarmById(alarmId);
      for(int i = 0; i < m_actions.size(); i++)
      {
         const ActionExecutionConfiguration *a = m_actions.get(i);
         bool execute = true;
         if (a->blockingTimerKey != nullptr && a->blockingTimerKey[0] != 0)
         {
            String key = event->expandText(a->blockingTimerKey, alarm);
            if (CountScheduledTasksByKey(key) > 0)
            {
               execute = false;
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Action %u execution blocked by timer \"%s\" key"), a->actionId, (const TCHAR *)key);
            }
         }

         if (execute)
         {
            uint64_t timerDelay = _tcstoul(event->expandText(a->timerDelay, alarm).cstr(), nullptr, 10);
            if (timerDelay == 0)
            {
               ExecuteAction(a->actionId, event, alarm);
            }
            else
            {
               TCHAR parameters[64], comments[256];
               _sntprintf(parameters, 64, _T("action=%u;event=") UINT64_FMT _T(";alarm=%u"), a->actionId, event->getId(), (alarm != nullptr) ? alarm->getAlarmId() : 0);
               _sntprintf(comments, 256, _T("Delayed action execution for event %s"), event->getName());
               String key = ((a->timerKey != nullptr) && (*a->timerKey != 0)) ? event->expandText(a->timerKey, alarm) : String();
               AddOneTimeScheduledTask(_T("Execute.Action"), time(nullptr) + timerDelay, parameters,
                        new ActionExecutionTransientData(event, alarm), 0, event->getSourceId(), SYSTEM_ACCESS_FULL,
                        comments, key.isEmpty() ? nullptr : key.cstr(), true);
            }

            uint64_t snoozeTime = _tcstoul(event->expandText(a->snoozeTime, alarm).cstr(), nullptr, 10);
            if (snoozeTime != 0 && a->blockingTimerKey != nullptr && a->blockingTimerKey[0] != 0)
            {
               TCHAR parameters[64], comments[256];
               _sntprintf(parameters, 64, _T("action=%u;event=") UINT64_FMT _T(";alarm=%u"), a->actionId, event->getId(), (alarm != nullptr) ? alarm->getAlarmId() : 0);
               _sntprintf(comments, 256, _T("Snooze action execution for event %s"), event->getName());
               String key = ((a->blockingTimerKey != nullptr) && (*a->blockingTimerKey != 0)) ? event->expandText(a->blockingTimerKey, alarm) : String();
               AddOneTimeScheduledTask(_T("Dummy"), time(nullptr) + snoozeTime, parameters,
                        nullptr, 0, event->getSourceId(), SYSTEM_ACCESS_FULL, comments, key.isEmpty() ? nullptr : key.cstr(), true);
            }
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
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Delayed action execution with key \"%s\" cancelled"), key.cstr());
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
uint32_t EPRule::generateAlarm(Event *event) const
{
   uint32_t alarmId = 0;
   String key = event->expandText(m_alarmKey);

   // Terminate alarms with key == our ack_key
   if ((m_alarmSeverity == SEVERITY_RESOLVE) || (m_alarmSeverity == SEVERITY_TERMINATE))
   {
      if (!key.isEmpty())
         ResolveAlarmByKey(key, (m_flags & RF_TERMINATE_BY_REGEXP) ? true : false, m_alarmSeverity == SEVERITY_TERMINATE, event);
   }
   else	// Generate new alarm
   {
      uint32_t parentAlarmId = 0;
      if ((m_rcaScriptName != nullptr) && (m_rcaScriptName[0] != 0))
	   {
	      shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
	      NXSL_VM *vm = CreateServerScriptVM(m_rcaScriptName, object);
	      if (vm != nullptr)
	      {
	         vm->setGlobalVariable("$event", vm->createValue(vm->createObject(&g_nxslEventClass, event, true)));
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
	            ReportScriptError(SCRIPT_CONTEXT_EVENT_PROC, FindObjectById(event->getSourceId()).get(), 0, vm->getErrorText(), m_rcaScriptName);
	            nxlog_write(NXLOG_ERROR, _T("Failed to execute root cause analysis script for event processing policy rule #%u (%s)"), m_id + 1, vm->getErrorText());
	         }
	         delete vm;
	      }
	   }
	   String message = event->expandText(m_alarmMessage);
	   String impact = event->expandText(m_alarmImpact);

	   alarmId = CreateNewAlarm(m_guid, m_comments, message, key, impact,
                     (m_alarmSeverity == SEVERITY_FROM_EVENT) ? event->getSeverity() : m_alarmSeverity,
                     m_alarmTimeout, m_alarmTimeoutEvent, parentAlarmId, m_rcaScriptName, event, 0, m_alarmCategoryList,
                     ((m_flags & RF_CREATE_TICKET) != 0) ? true : false);
	   event->setLastAlarmMessage(message);
	}

   event->setLastAlarmKey(key);
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
   if (hResult != nullptr)
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
   if (hResult != nullptr)
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
   _sntprintf(szQuery, 256, _T("SELECT action_id,timer_delay,timer_key,blocking_timer_key,snooze_time FROM policy_action_list WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t actionId = DBGetFieldULong(hResult, i, 0);
         TCHAR *timerDelay = DBGetField(hResult, i, 1, nullptr, 0);
         TCHAR *timerKey = DBGetField(hResult, i, 2, nullptr, 0);
         TCHAR *blockingTimerKey = DBGetField(hResult, i, 3, nullptr, 0);
         TCHAR *snoozeTime = DBGetField(hResult, i, 4, nullptr, 0);
         m_actions.add(new ActionExecutionConfiguration(actionId, timerDelay, snoozeTime, timerKey, blockingTimerKey));
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
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         m_timerCancellations.addPreallocated(DBGetField(hResult, i, 0, nullptr, 0));
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
   if (hResult != nullptr)
   {
      TCHAR key[MAX_DB_STRING];
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, key, MAX_DB_STRING);
         if (DBGetFieldULong(hResult, i, 1) == PSTORAGE_SET)
         {
            m_pstorageSetActions.setPreallocated(_tcsdup(key), DBGetField(hResult, i, 2, nullptr, 0));
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
   if (hResult != nullptr)
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
bool EPRule::saveToDB(DB_HANDLE hdb) const
{
   bool success;
	int i;
	TCHAR pszQuery[1024];
   // General attributes
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_impact,")
                                  _T("alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event,rca_script_name) ")
                                  _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?)"));
   if (hStmt != nullptr)
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
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO policy_action_list (rule_id,action_id,timer_delay,timer_key,blocking_timer_key,snooze_time) VALUES (?,?,?,?,?,?)"), m_actions.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(i = 0; i < m_actions.size() && success; i++)
         {
            const ActionExecutionConfiguration *a = m_actions.get(i);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, a->actionId);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, a->timerDelay, DB_BIND_STATIC, 127);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, a->timerKey, DB_BIND_STATIC, 127);
            DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, a->blockingTimerKey, DB_BIND_STATIC, 127);
            DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, a->snoozeTime, DB_BIND_STATIC, 127);
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
      if (hStmt != nullptr)
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
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         success = _STOP != m_pstorageSetActions.forEach(SavePstorageSetActions, hStmt);
         DBFreeStatement(hStmt);
      }
   }

   if (success && !m_pstorageDeleteActions.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO policy_pstorage_actions (rule_id,action,ps_key) VALUES (?,2,?)"), m_pstorageDeleteActions.size() > 1);
      if (hStmt != nullptr)
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
      if (hStmt != nullptr)
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
void EPRule::createMessage(NXCPMessage *msg) const
{
   msg->setField(VID_FLAGS, m_flags);
   msg->setField(VID_RULE_ID, m_id);
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_ALARM_SEVERITY, static_cast<int16_t>(m_alarmSeverity));
   msg->setField(VID_ALARM_KEY, m_alarmKey);
   msg->setField(VID_ALARM_MESSAGE, m_alarmMessage);
   msg->setField(VID_IMPACT, m_alarmImpact);
   msg->setField(VID_ALARM_TIMEOUT, m_alarmTimeout);
   msg->setField(VID_ALARM_TIMEOUT_EVENT, m_alarmTimeoutEvent);
   msg->setFieldFromInt32Array(VID_ALARM_CATEGORY_ID, &m_alarmCategoryList);
   msg->setField(VID_RCA_SCRIPT_NAME, m_rcaScriptName);
   msg->setField(VID_COMMENTS, CHECK_NULL_EX(m_comments));
   msg->setField(VID_NUM_ACTIONS, static_cast<uint32_t>(m_actions.size()));
   uint32_t fieldId = VID_ACTION_LIST_BASE;
   for(int i = 0; i < m_actions.size(); i++)
   {
      const ActionExecutionConfiguration *a = m_actions.get(i);
      msg->setField(fieldId++, a->actionId);
      msg->setField(fieldId++, a->timerDelay);
      msg->setField(fieldId++, a->timerKey);
      msg->setField(fieldId++, a->blockingTimerKey);
      msg->setField(fieldId++, a->snoozeTime);
      fieldId += 5;
   }
   m_timerCancellations.fillMessage(msg, VID_TIMER_LIST_BASE, VID_TIMER_COUNT);
   msg->setField(VID_NUM_EVENTS, (UINT32)m_events.size());
   msg->setFieldFromInt32Array(VID_RULE_EVENTS, &m_events);
   msg->setField(VID_NUM_SOURCES, (UINT32)m_sources.size());
   msg->setFieldFromInt32Array(VID_RULE_SOURCES, &m_sources);
   msg->setField(VID_SCRIPT, CHECK_NULL_EX(m_scriptSource));
   m_pstorageSetActions.fillMessage(msg, VID_PSTORAGE_SET_LIST_BASE, VID_NUM_SET_PSTORAGE);
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
      json_object_set_new(action, "timerDelay", json_string_t(d->timerDelay));
      json_object_set_new(action, "timerKey", json_string_t(d->timerKey));
      json_object_set_new(action, "blockingTimerKey", json_string_t(d->blockingTimerKey));
      json_object_set_new(action, "snoozeTime", json_string_t(d->snoozeTime));
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
   if (hResult != nullptr)
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
void EventPolicy::sendToClient(ClientSession *session, uint32_t requestId) const
{
   NXCPMessage msg(CMD_EPP_RECORD, requestId);

   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      m_rules.get(i)->createMessage(&msg);
      session->sendMessage(&msg);
      msg.deleteAllFields();
   }
   unlock();
}

/**
 * Replace policy with new one
 */
void EventPolicy::replacePolicy(uint32_t numRules, EPRule **ruleList)
{
   writeLock();
   m_rules.clear();
   if (ruleList != nullptr)
   {
      for(int i = 0; i < (int)numRules; i++)
      {
         EPRule *r = ruleList[i];
         r->setId(i);
         m_rules.add(r);
      }
   }
   unlock();
}

/**
 * Check if given action is used in policy
 */
bool EventPolicy::isActionInUse(uint32_t actionId) const
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
bool EventPolicy::isCategoryInUse(uint32_t categoryId) const
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
      if (ruleOrdering != nullptr)
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

         if (newRulePrevIndex != -1)
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

      if (ruleIndex == -1) // Add new rule at the end
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

/**
 * Collects information about all EPRules that are using specified event
 */
void EventPolicy::getEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences) const
{
   readLock();
   for (int i = 0; i < m_rules.size(); i++)
   {
      EPRule *rule = m_rules.get(i);
      if (rule->isUsingEvent(eventCode))
      {
         eventReferences->add(new EventReference(EventReferenceType::EP_RULE, i + 1, rule->getGuid(), rule->getComments()));
      }
   }
   unlock();
}
