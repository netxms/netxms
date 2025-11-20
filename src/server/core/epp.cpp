/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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

void StartDowntime(uint32_t objectId, String tag);
void EndDowntime(uint32_t objectId, String tag);

void ProcessEventWithAIAssistant(Event *event, const shared_ptr<NetObj>& object, const wchar_t *instructions);

/**
 * Default event policy rule constructor
 */
EPRule::EPRule(uint32_t id) : m_timeFrames(0, 16, Ownership::True), m_actions(0, 16, Ownership::True)
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
   m_filterScriptSource = nullptr;
   m_filterScript = nullptr;
   m_actionScriptSource = nullptr;
   m_actionScript = nullptr;
	m_alarmTimeout = 0;
	m_alarmTimeoutEvent = EVENT_ALARM_TIMEOUT;
	m_downtimeTag[0] = 0;
	m_aiAgentInstructions = nullptr;
}

/**
 * Create rule from config entry
 */
EPRule::EPRule(const ConfigEntry& config, ImportContext *context, bool nxslV5) : m_timeFrames(0, 16, Ownership::True), m_actions(0, 16, Ownership::True)
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
         else
         {
            context->log(NXLOG_WARNING, _T("EPRule::EPRule()"),
               _T("Event processing policy rule import: rule \"%s\" refers to unknown event template \"%s\""),
               m_guid.toString().cstr(),
               events->get(i)->getSubEntryValue(_T("name"), 0, _T("<unknown>")));
         }
      }
   }

   ConfigEntry *timeFrameRoot = config.findEntry(_T("timeFrames"));
   if (timeFrameRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> frames = timeFrameRoot->getSubEntries(_T("timeFrame#*"));
      for(int i = 0; i < frames->size(); i++)
      {
         uint32_t time = frames->get(i)->getAttributeAsUInt(L"time");
         uint64_t date = frames->get(i)->getAttributeAsUInt64(L"date");
         m_timeFrames.add(new TimeFrame(time, date));
      }
   }

   m_comments = MemCopyString(config.getSubEntryValue(_T("comments"), 0, _T("")));
   m_alarmSeverity = config.getSubEntryValueAsInt(_T("alarmSeverity"));
	m_alarmTimeout = config.getSubEntryValueAsUInt(_T("alarmTimeout"));
	m_alarmTimeoutEvent = config.getSubEntryValueAsUInt(_T("alarmTimeoutEvent"), 0, EVENT_ALARM_TIMEOUT);
   m_alarmKey = MemCopyString(config.getSubEntryValue(_T("alarmKey")));
   m_alarmMessage = MemCopyString(config.getSubEntryValue(_T("alarmMessage")));
   m_alarmImpact = MemCopyString(config.getSubEntryValue(_T("alarmImpact")));
   m_rcaScriptName = MemCopyString(config.getSubEntryValue(L"rootCauseAnalysisScript"));
   m_aiAgentInstructions = MemCopyString(config.getSubEntryValue(L"aiAgentInstructions"));

   wcslcpy(m_downtimeTag, config.getSubEntryValue(L"downtimeTag", 0, L""), MAX_DOWNTIME_TAG_LENGTH);

   ConfigEntry *pStorageEntry = config.findEntry(L"pStorageActions");
   if (pStorageEntry != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> tmp = pStorageEntry->getSubEntries(L"set#*");
      for(int i = 0; i < tmp->size(); i++)
      {
         m_pstorageSetActions.set(tmp->get(i)->getAttribute(L"key"), tmp->get(i)->getValue());
      }

      tmp = pStorageEntry->getSubEntries(L"delete#*");
      for(int i = 0; i < tmp->size(); i++)
      {
         m_pstorageDeleteActions.add(tmp->get(i)->getAttribute(L"key"));
      }
   }

   ConfigEntry *customAttributeEntry = config.findEntry(L"customAttributeActions");
   if (customAttributeEntry != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> tmp = customAttributeEntry->getSubEntries(_T("set#*"));
      for(int i = 0; i < tmp->size(); i++)
      {
         m_customAttributeSetActions.set(tmp->get(i)->getAttribute(_T("name")), tmp->get(i)->getValue());
      }

      tmp = customAttributeEntry->getSubEntries(_T("delete#*"));
      for(int i = 0; i < tmp->size(); i++)
      {
         m_customAttributeDeleteActions.add(tmp->get(i)->getAttribute(_T("name")));
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
            const TCHAR *name = categories->get(i)->getAttribute(L"name");
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

   if (nxslV5)
      m_filterScriptSource = MemCopyString(config.getSubEntryValue(_T("script"), 0, _T("")));
   else
   {
      StringBuffer output = NXSLConvertToV5(config.getSubEntryValue(_T("script"), 0, _T("")));
      m_filterScriptSource = MemCopyString(output);
   }
   if ((m_filterScriptSource != nullptr) && (*m_filterScriptSource != 0))
   {
      m_filterScript = CompileServerScript(m_filterScriptSource, SCRIPT_CONTEXT_EVENT_PROC, nullptr, 0, _T("EPP::Filter::%u"), m_id + 1);
      if (m_filterScript == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for event processing policy rule #%u"), m_id + 1);
      }
   }
   else
   {
      m_filterScript = nullptr;
   }

   if (nxslV5)
      m_actionScriptSource = MemCopyString(config.getSubEntryValue(_T("actionScript"), 0, _T("")));
   else
   {
      StringBuffer output = NXSLConvertToV5(config.getSubEntryValue(_T("actionScript"), 0, _T("")));
      m_actionScriptSource = MemCopyString(output);
   }
   if ((m_actionScriptSource != nullptr) && (*m_actionScriptSource != 0))
   {
      m_actionScript = CompileServerScript(m_actionScriptSource, SCRIPT_CONTEXT_EVENT_PROC, nullptr, 0, _T("EPP::Action::%u"), m_id + 1);
      if (m_actionScript == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile action script for event processing policy rule #%u"), m_id + 1);
      }
   }
   else
   {
      m_actionScript = nullptr;
   }

   ConfigEntry *actionsRoot = config.findEntry(L"actions");
   if (actionsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> actions = actionsRoot->getSubEntries(L"action#*");
      for(int i = 0; i < actions->size(); i++)
      {
         uuid guid = actions->get(i)->getSubEntryValueAsUUID(_T("guid"));
         const TCHAR *timerDelay = actions->get(i)->getSubEntryValue(_T("timerDelay"));
         const TCHAR *timerKey = actions->get(i)->getSubEntryValue(_T("timerKey"));
         const TCHAR *blockingTimerKey = actions->get(i)->getSubEntryValue(_T("blockingTimerKey"));
         const TCHAR *snoozeTime = actions->get(i)->getSubEntryValue(_T("snoozeTime"));
         bool active = actions->get(i)->getSubEntryValueAsBoolean(_T("active"), 0, true);
         if (!guid.isNull())
         {
            uint32_t actionId = FindActionByGUID(guid);
            if (actionId != 0)
               m_actions.add(new ActionExecutionConfiguration(actionId, MemCopyString(timerDelay), MemCopyString(snoozeTime), MemCopyString(timerKey), MemCopyString(blockingTimerKey), active));
         }
         else
         {
            uint32_t actionId = actions->get(i)->getId();
            if (IsValidActionId(actionId))
               m_actions.add(new ActionExecutionConfiguration(actionId, MemCopyString(timerDelay), MemCopyString(snoozeTime), MemCopyString(timerKey), MemCopyString(blockingTimerKey), active));
         }
      }
   }

   ConfigEntry *timerCancellationsRoot = config.findEntry(L"timerCancellations");
   if (timerCancellationsRoot != nullptr)
   {
      ConfigEntry *keys = timerCancellationsRoot->findEntry(L"timerKey");
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
 * alarm_timeout,alarm_timeout_event,rca_script_name,alarm_impact,action_script,
 * downtime_tag,ai_instructions
 */
EPRule::EPRule(DB_RESULT hResult, int row) : m_timeFrames(0, 16, Ownership::True), m_actions(0, 16, Ownership::True)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_guid = DBGetFieldGUID(hResult, row, 1);
   m_flags = DBGetFieldULong(hResult, row, 2);
   m_comments = DBGetField(hResult, row, 3, nullptr, 0);
   m_alarmMessage = DBGetField(hResult, row, 4, nullptr, 0);
   m_alarmSeverity = DBGetFieldLong(hResult, row, 5);
   m_alarmKey = DBGetField(hResult, row, 6, nullptr, 0);
   m_filterScriptSource = DBGetField(hResult, row, 7, nullptr, 0);
   if ((m_filterScriptSource != nullptr) && (*m_filterScriptSource != 0))
   {
      m_filterScript = CompileServerScript(m_filterScriptSource, SCRIPT_CONTEXT_EVENT_PROC, nullptr, 0, _T("EPP::Filter::%u"), m_id + 1);
      if (m_filterScript == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for event processing policy rule #%u"), m_id + 1);
      }
   }
   else
   {
      m_filterScript = nullptr;
   }
	m_alarmTimeout = DBGetFieldULong(hResult, row, 8);
	m_alarmTimeoutEvent = DBGetFieldULong(hResult, row, 9);
	m_rcaScriptName = DBGetField(hResult, row, 10, nullptr, 0);
   m_alarmImpact = DBGetField(hResult, row, 11, nullptr, 0);
   m_actionScriptSource = DBGetField(hResult, row, 12, nullptr, 0);
   if ((m_actionScriptSource != nullptr) && (*m_actionScriptSource != 0))
   {
      m_actionScript = CompileServerScript(m_actionScriptSource, SCRIPT_CONTEXT_EVENT_PROC, nullptr, 0, _T("EPP::Action::%u"), m_id + 1);
      if (m_actionScript == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile action script for event processing policy rule #%u"), m_id + 1);
      }
   }
   else
   {
      m_actionScript = nullptr;
   }
   DBGetField(hResult, row, 13, m_downtimeTag, MAX_DOWNTIME_TAG_LENGTH);
   m_aiAgentInstructions = DBGetField(hResult, row, 14, nullptr, 0);
}

/**
 * Construct event policy rule from NXCP message
 */
EPRule::EPRule(const NXCPMessage& msg) : m_timeFrames(0, 16, Ownership::True), m_actions(0, 16, Ownership::True)
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
         m_actions.add(new ActionExecutionConfiguration(actions.get(i), nullptr, nullptr, nullptr, nullptr, true));
      }
   }
   else
   {
      int count = msg.getFieldAsInt32(VID_NUM_ACTIONS);
      uint32_t fieldId = VID_ACTION_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         uint32_t actionId = msg.getFieldAsUInt32(fieldId++);
         wchar_t *timerDelay = msg.getFieldAsString(fieldId++);
         wchar_t *timerKey = msg.getFieldAsString(fieldId++);
         wchar_t *blockingTimerKey = msg.getFieldAsString(fieldId++);
         wchar_t *snoozeTime = msg.getFieldAsString(fieldId++);
         bool active = msg.getFieldAsBoolean(fieldId++);
         fieldId += 4;
         m_actions.add(new ActionExecutionConfiguration(actionId, timerDelay, snoozeTime, timerKey, blockingTimerKey, active));
      }
   }

   if (msg.isFieldExist(VID_TIMER_LIST))
   {
      m_timerCancellations.addAllFromMessage(msg, VID_TIMER_LIST);
   }

   msg.getFieldAsInt32Array(VID_RULE_EVENTS, &m_events);
   msg.getFieldAsInt32Array(VID_RULE_SOURCES, &m_sources);
   msg.getFieldAsInt32Array(VID_RULE_SOURCE_EXCLUSIONS, &m_sourceExclusions);

   int count = msg.getFieldAsInt32(VID_NUM_TIME_FRAMES);
   uint32_t fieldId = VID_TIME_FRAME_LIST_BASE;
   for(int i = 0; i < count; i++)
   {
      m_timeFrames.add(new TimeFrame(msg.getFieldAsUInt32(fieldId), msg.getFieldAsUInt64(fieldId + 1)));
      fieldId += 2;
   }

   m_alarmKey = msg.getFieldAsString(VID_ALARM_KEY);
   m_alarmMessage = msg.getFieldAsString(VID_ALARM_MESSAGE);
   m_alarmImpact = msg.getFieldAsString(VID_IMPACT);
   m_alarmSeverity = msg.getFieldAsUInt16(VID_ALARM_SEVERITY);
	m_alarmTimeout = msg.getFieldAsUInt32(VID_ALARM_TIMEOUT);
	m_alarmTimeoutEvent = msg.getFieldAsUInt32(VID_ALARM_TIMEOUT_EVENT);
	m_rcaScriptName = msg.getFieldAsString(VID_RCA_SCRIPT_NAME);

   msg.getFieldAsInt32Array(VID_ALARM_CATEGORY_ID, &m_alarmCategoryList);

   msg.getFieldAsString(VID_DOWNTIME_TAG, m_downtimeTag, MAX_DOWNTIME_TAG_LENGTH);

   m_pstorageSetActions.addAllFromMessage(msg, VID_PSTORAGE_SET_LIST_BASE, VID_NUM_SET_PSTORAGE);
   m_pstorageDeleteActions.addAllFromMessage(msg, VID_PSTORAGE_DELETE_LIST_BASE, VID_NUM_DELETE_PSTORAGE);

   m_customAttributeSetActions.addAllFromMessage(msg, VID_CUSTOM_ATTR_SET_LIST_BASE, VID_CUSTOM_ATTR_SET_COUNT);
   m_customAttributeDeleteActions.addAllFromMessage(msg, VID_CUSTOM_ATTR_DEL_LIST_BASE, VID_CUSTOM_ATTR_DEL_COUNT);

   m_filterScriptSource = msg.getFieldAsString(VID_SCRIPT);
   if ((m_filterScriptSource != nullptr) && (*m_filterScriptSource != 0))
   {
      m_filterScript = CompileServerScript(m_filterScriptSource, SCRIPT_CONTEXT_EVENT_PROC, nullptr, 0, _T("EPP::Filter::%u"), m_id + 1);
      if (m_filterScript == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile evaluation script for event processing policy rule #%u"), m_id + 1);
      }
   }
   else
   {
      m_filterScript = nullptr;
   }

   m_actionScriptSource = msg.getFieldAsString(VID_ACTION_SCRIPT);
   if ((m_actionScriptSource != nullptr) && (*m_actionScriptSource != 0))
   {
      m_actionScript = CompileServerScript(m_actionScriptSource, SCRIPT_CONTEXT_EVENT_PROC, nullptr, 0, _T("EPP::Action::%u"), m_id + 1);
      if (m_actionScript == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to compile action script for event processing policy rule #%u"), m_id + 1);
      }
   }
   else
   {
      m_actionScript = nullptr;
   }

   m_aiAgentInstructions = msg.getFieldAsString(VID_AI_AGENT_INSTRUCTIONS);
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
   MemFree(m_filterScriptSource);
   delete m_filterScript;
   MemFree(m_actionScriptSource);
   delete m_actionScript;
   MemFree(m_aiAgentInstructions);
}

/**
 * Create rule ordering entry
 */
void EPRule::createOrderingExportRecord(TextFileWriter& xml) const
{
   xml.appendUtf8String("\t\t<rule id=\"");
   xml.append(m_id + 1);
   xml.appendUtf8String("\">");
   xml.append(m_guid.toString());
   xml.appendUtf8String("</rule>\n");
}

/**
 * Create management pack record
 */
void EPRule::createExportRecord(TextFileWriter& xml) const
{
   xml.appendUtf8String("\t\t<rule id=\"");
   xml.append(m_id + 1);
   xml.appendUtf8String("\">\n\t\t\t<guid>");
   xml.append(m_guid);
   xml.appendUtf8String("</guid>\n\t\t\t<flags>");
   xml.append(m_flags);
   xml.appendUtf8String("</flags>\n\t\t\t<alarmMessage>");
   xml.append(EscapeStringForXML2(m_alarmMessage));
   xml.appendUtf8String("</alarmMessage>\n\t\t\t<alarmImpact>");
   xml.append(EscapeStringForXML2(m_alarmImpact));
   xml.appendUtf8String("</alarmImpact>\n\t\t\t<alarmKey>");
   xml.append(EscapeStringForXML2(m_alarmKey));
   xml.appendUtf8String("</alarmKey>\n\t\t\t<rootCauseAnalysisScript>");
   xml.append(EscapeStringForXML2(m_rcaScriptName));
   xml.appendUtf8String("</rootCauseAnalysisScript>\n\t\t\t<alarmSeverity>");
   xml.append(m_alarmSeverity);
   xml.appendUtf8String("</alarmSeverity>\n\t\t\t<alarmTimeout>");
   xml.append(m_alarmTimeout);
   xml.appendUtf8String("</alarmTimeout>\n\t\t\t<alarmTimeoutEvent>");
   xml.append(m_alarmTimeoutEvent);
   xml.appendUtf8String("</alarmTimeoutEvent>\n\t\t\t<downtimeTag>");
   xml.append(EscapeStringForXML2(m_downtimeTag));
   xml.appendUtf8String("</downtimeTag>\n\t\t\t<script>");
   xml.append(EscapeStringForXML2(m_filterScriptSource));
   xml.appendUtf8String("</script>\n\t\t\t<actionScript>");
   xml.append(EscapeStringForXML2(m_actionScriptSource));
   xml.appendUtf8String("</actionScript>\n\t\t\t<comments>");
   xml.append(EscapeStringForXML2(m_comments));
   xml.appendUtf8String("</comments>\n\t\t\t<sources>\n");

   for(int i = 0; i < m_sources.size(); i++)
   {
      shared_ptr<NetObj> object = FindObjectById(m_sources.get(i));
      if (object != nullptr)
      {
         xml.appendUtf8String("\t\t\t\t<source id=\"");
         xml.append(object->getId());
         xml.appendUtf8String("\">\n\t\t\t\t\t<name>");
         xml.append(EscapeStringForXML2(object->getName()));
         xml.appendUtf8String("</name>\n\t\t\t\t\t<guid>");
         xml.append(object->getGuid());
         xml.appendUtf8String("</guid>\n\t\t\t\t\t<class>");
         xml.append(object->getObjectClass());
         xml.appendUtf8String("</class>\n\t\t\t\t</source>\n");
      }
   }
   xml.appendUtf8String("\t\t\t</sources>\n\t\t\t<sourceExclusions>\n");

   for(int i = 0; i < m_sourceExclusions.size(); i++)
   {
      shared_ptr<NetObj> object = FindObjectById(m_sourceExclusions.get(i));
      if (object != nullptr)
      {
         xml.appendUtf8String("\t\t\t\t<sourceExclusion id=\"");
         xml.append(object->getId());
         xml.appendUtf8String("\">\n\t\t\t\t\t<name>");
         xml.append(EscapeStringForXML2(object->getName()));
         xml.appendUtf8String("</name>\n\t\t\t\t\t<guid>");
         xml.append(object->getGuid());
         xml.appendUtf8String("</guid>\n\t\t\t\t\t<class>");
         xml.append(object->getObjectClass());
         xml.appendUtf8String("</class>\n\t\t\t\t</sourceExclusion>\n");
      }
   }

   xml.appendUtf8String("\t\t\t</sourceExclusions>\n\t\t\t<events>\n");

   for(int i = 0; i < m_events.size(); i++)
   {
      xml.appendUtf8String("\t\t\t\t<event id=\"");
      xml.append(m_events.get(i));
      xml.appendUtf8String("\">\n\t\t\t\t\t<name>");
      wchar_t eventName[MAX_EVENT_NAME];
      EventNameFromCode(m_events.get(i), eventName);
      xml.append(EscapeStringForXML2(eventName));
      xml.appendUtf8String("</name>\n\t\t\t\t</event>\n");
   }

   xml.appendUtf8String("\t\t\t</events>\n\t\t\t<timeFrames>\n");

   for(int i = 0; i < m_timeFrames.size(); i++)
   {
      TimeFrame *timeFrame = m_timeFrames.get(i);
      xml.appendUtf8String("\t\t\t\t<timeFrame id=\"");
      xml.append(i + 1);
      xml.appendUtf8String("\" time=\"");
      xml.append(timeFrame->getTimeFilter());
      xml.appendUtf8String("\" date=\"");
      xml.append(timeFrame->getDateFilter());
      xml.appendUtf8String("\" />\n");
   }

   xml.appendUtf8String("\t\t\t</timeFrames>\n\t\t\t<actions>\n");
   for(int i = 0; i < m_actions.size(); i++)
   {
      xml.append(_T("\t\t\t\t<action id=\""));
      const ActionExecutionConfiguration *a = m_actions.get(i);
      xml.append(a->actionId);
      xml.appendUtf8String("\">\n\t\t\t\t\t<guid>");
      xml.append(GetActionGUID(a->actionId));
      xml.appendUtf8String("</guid>\n\t\t\t\t\t<timerDelay>");
      xml.append(EscapeStringForXML2(a->timerDelay));
      xml.appendUtf8String("</timerDelay>\n\t\t\t\t\t<timerKey>");
      xml.append(EscapeStringForXML2(a->timerKey));
      xml.appendUtf8String("</timerKey>\n\t\t\t\t\t<blockingTimerKey>");
      xml.append(EscapeStringForXML2(a->blockingTimerKey));
      xml.appendUtf8String("</blockingTimerKey>\n\t\t\t\t\t<snoozeTime>");
      xml.append(EscapeStringForXML2(a->snoozeTime));
      xml.appendUtf8String("</snoozeTime>\n\t\t\t\t\t<active>");
      xml.append(a->active);
      xml.appendUtf8String("</active>\n\t\t\t\t</action>\n");
   }

   xml.appendUtf8String("\t\t\t</actions>\n\t\t\t<timerCancellations>\n");
   for(int i = 0; i < m_timerCancellations.size(); i++)
   {
      xml.appendUtf8String("\t\t\t\t<timerKey>");
      xml.append(EscapeStringForXML2(m_timerCancellations.get(i)));
      xml.appendUtf8String("</timerKey>\n");
   }

   xml.appendUtf8String("\t\t\t</timerCancellations>\n\t\t\t<pStorageActions>\n");
   int id = 0;
   for(KeyValuePair<const wchar_t> *action : m_pstorageSetActions)
   {
      xml.appendUtf8String("\t\t\t\t<set id=\"");
      xml.append(++id);
      xml.appendUtf8String("\" key=\"");
      xml.append(EscapeStringForXML2(action->key));
      xml.appendUtf8String("\">");
      xml.append(EscapeStringForXML2(action->value));
      xml.appendUtf8String("</set>\n");
   }
   for(int i = 0; i < m_pstorageDeleteActions.size(); i++)
   {
      xml.appendUtf8String("\t\t\t\t<delete id=\"");
      xml.append(i + 1);
      xml.appendUtf8String("\" key=\"");
      xml.append(EscapeStringForXML2(m_pstorageDeleteActions.get(i)));
      xml.appendUtf8String("\"/>\n");
   }

   xml.appendUtf8String("\t\t\t</pStorageActions>\n\t\t\t<customAttributeActions>\n");
   id = 0;
   for(KeyValuePair<const wchar_t> *action : m_customAttributeSetActions)
   {
      xml.appendUtf8String("\t\t\t\t<set id=\"");
      xml.append(++id);
      xml.appendUtf8String("\" name=\"");
      xml.append(EscapeStringForXML2(action->key));
      xml.appendUtf8String("\">");
      xml.append(EscapeStringForXML2(action->value));
      xml.appendUtf8String("</set>\n");
   }
   for(int i = 0; i < m_customAttributeDeleteActions.size(); i++)
   {
      xml.appendUtf8String("\t\t\t\t<delete id=\"");
      xml.append(i + 1);
      xml.appendUtf8String("\" name=\"");
      xml.append(EscapeStringForXML2(m_customAttributeDeleteActions.get(i)));
      xml.appendUtf8String("\"/>\n");
   }

   xml.appendUtf8String("\t\t\t</customAttributeActions>\n\t\t\t<alarmCategories>\n");
   for(int i = 0; i < m_alarmCategoryList.size(); i++)
   {
      AlarmCategory *category = GetAlarmCategory(m_alarmCategoryList.get(i));
      xml.appendUtf8String("\t\t\t\t<category id=\"");
      xml.append(category->getId());
      xml.appendUtf8String("\" name=\"");
      xml.append(EscapeStringForXML2(category->getName()));
      xml.appendUtf8String("\">");
      xml.append(category->getDescription());
      xml.appendUtf8String("</category>\n");
      delete category;
   }

   if ((m_aiAgentInstructions != nullptr) && (*m_aiAgentInstructions != 0))
   {
      xml.appendUtf8String("\t\t\t\t<aiAgentInstructions>");
      xml.append(EscapeStringForXML2(m_aiAgentInstructions));
      xml.appendUtf8String("</aiAgentInstructions>\n");
   }

   xml.appendUtf8String("\t\t\t</alarmCategories>\n\t\t</rule>\n");
}

/**
 * Validate rule configuration
 */
void EPRule::validateConfig() const
{
   const uint32_t ruleId = getId() + 1;

   for(int i = 0; i < m_sourceExclusions.size(); i++)
   {
      const uint32_t objectId = m_sourceExclusions.get(i);
      if (FindObjectById(objectId) == nullptr)
      {
         ReportConfigurationError(L"EPP", L"invalid-object-id", L"Invalid object ID %u in EPP rule %u", objectId, ruleId);
      }
   }

   for(int i = 0; i < m_sources.size(); i++)
   {
      const uint32_t objectId = m_sources.get(i);
      if (FindObjectById(objectId) == nullptr)
      {
         ReportConfigurationError(L"EPP", L"invalid-object-id", L"Invalid object ID %u in EPP rule %u", objectId, ruleId);
      }
   }
}

/**
 * Check if source object's id match to the rule
 */
bool EPRule::matchSource(const shared_ptr<NetObj>& object) const
{
   if (m_sources.isEmpty() && m_sourceExclusions.isEmpty())
      return (m_flags & RF_NEGATED_SOURCE) ? false : true;

   if (object == nullptr)
      return (m_flags & RF_NEGATED_SOURCE) ? true : false;

   uint32_t objectId = object->getId();
   bool exception = false;
   for(int i = 0; i < m_sourceExclusions.size(); i++)
   {
      uint32_t id = m_sourceExclusions.get(i);
      if (id == objectId)
      {
         exception = true;
         break;
      }

      if (object->isParent(id))
      {
         exception = true;
         break;
      }
   }
   if (exception)
   {
      return (m_flags & RF_NEGATED_SOURCE) ? true : false;
   }

   bool match = m_sources.isEmpty();
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
 * Check if execution time matches the frames
 */
bool EPRule::matchTime(struct tm *localTime) const
{
   if (m_timeFrames.isEmpty())
      return (m_flags & RF_NEGATED_TIME_FRAMES) ? false : true;

   bool match = false;
   uint32_t currentTime = localTime->tm_hour * 100 + localTime->tm_min; // BCD format
   for(TimeFrame *frame : m_timeFrames)
   {
      if (frame->match(localTime, currentTime))
      {
         match = true;
         break;
      }
   }
   return (m_flags & RF_NEGATED_TIME_FRAMES) ? !match : match;
}

/**
 * Check if event match to the script
 */
void EPRule::executeActionScript(Event *event) const
{
   if (m_actionScript == nullptr)
      return;

   shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
   shared_ptr<DCObjectInfo> dciInfo;
   if ((event->getDciId() != 0) && (object != nullptr) && object->isDataCollectionTarget())
   {
      shared_ptr<DCObject> dci = static_cast<DataCollectionTarget&>(*object).getDCObjectById(event->getDciId(), 0);
      if (dci != nullptr)
         dciInfo = dci->createDescriptor();
      else
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Event references DCI [%u] but it cannot be found in event source object"), event->getDciId());
   }

   ScriptVMHandle vm = CreateServerScriptVM(m_actionScript, object, dciInfo);
   if (!vm.isValid())
   {
      if (vm.failureReason() != ScriptVMFailureReason::SCRIPT_IS_EMPTY)
      {
         ReportScriptError(SCRIPT_CONTEXT_EVENT_PROC, object.get(), 0, _T("Script load failed"), _T("EPP::%d"), m_id + 1);
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot create NXSL VM for action script for event processing policy rule #%u"), m_id + 1);
      }
      return;
   }

   vm->setGlobalVariable("$event", vm->createValue(vm->createObject(&g_nxslEventClass, event, true)));

   // Pass event's parameters as arguments and
   // other information as variables
   ObjectRefArray<NXSL_Value> args(event->getParametersCount());
   for(int i = 0; i < event->getParametersCount(); i++)
      args.add(vm->createValue(event->getParameter(i)));

   // Run script
   NXSL_VariableSystem *globals = nullptr;
   if (!vm->run(args, &globals))
   {
      ReportScriptError(SCRIPT_CONTEXT_EVENT_PROC, object.get(), 0, vm->getErrorText(), _T("EPP::%d"), m_id + 1);
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Failed to execute action script for event processing policy rule #%u (%s)"), m_id + 1, vm->getErrorText());
   }
   delete globals;
   vm.destroy();
}

/**
 * Check if event match to the script
 */
bool EPRule::matchScript(Event *event) const
{
   if (m_filterScript == nullptr)
      return true;

   shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
   shared_ptr<DCObjectInfo> dciInfo;
   if ((event->getDciId() != 0) && (object != nullptr) && object->isDataCollectionTarget())
   {
      shared_ptr<DCObject> dci = static_cast<DataCollectionTarget&>(*object).getDCObjectById(event->getDciId(), 0);
      if (dci != nullptr)
         dciInfo = dci->createDescriptor();
      else
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Event references DCI [%u] but it cannot be found in event source object"), event->getDciId());
   }
   ScriptVMHandle vm = CreateServerScriptVM(m_filterScript, FindObjectById(event->getSourceId()), dciInfo);
   if (!vm.isValid())
   {
      if (vm.failureReason() != ScriptVMFailureReason::SCRIPT_IS_EMPTY)
      {
         ReportScriptError(SCRIPT_CONTEXT_EVENT_PROC, FindObjectById(event->getSourceId()).get(), 0, _T("Script load failed"), _T("EPP::%d"), m_id + 1);
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot create NXSL VM for evaluation script for event processing policy rule #%u"), m_id + 1);
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
   bool result = true;
   NXSL_VariableSystem *globals = nullptr;
   if (vm->run(args, &globals))
   {
      NXSL_Value *value = vm->getResult();
      result = value->getValueAsBoolean();
      if (result)
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
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Failed to execute filter script for event processing policy rule #%u (%s)"), m_id + 1, vm->getErrorText());
   }
   delete globals;
   vm.destroy();

   return result;
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

   // Check if rule has no filters at all
   if (isFilterEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("EPP rule %u ignored because filter is empty"), m_id + 1);
      return false;
   }

   if (!matchSeverity(event->getSeverity()) || !matchEvent(event->getCode()))
      return false;

   shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
   if (!matchSource(object))
      return false;

   time_t now = time(nullptr);
   struct tm currLocal;
#if HAVE_LOCALTIME_R
   localtime_r(&now, &currLocal);
#else
   memcpy(&currLocal, localtime(&now), sizeof(struct tm));
#endif
   if (!matchTime(&currLocal) || !matchScript(event))
      return false;

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Event ") UINT64_FMT _T(" match EPP rule %u"), event->getId(), m_id + 1);

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
         if (!a->isActive())
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("Action %u will not be executed, because id disabled"), a->actionId);
            continue;
         }
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
               ExecuteAction(a->actionId, *event, alarm, m_guid);
            }
            else
            {
               wchar_t parameters[128], comments[256];
               _sntprintf(parameters, 128, L"action=%u;event=" UINT64_FMT L";alarm=%u;rule=%s", a->actionId, event->getId(),
                     (alarm != nullptr) ? alarm->getAlarmId() : 0, m_guid.toString().cstr());
               _sntprintf(comments, 256, L"Delayed action execution for event %s", event->getName());
               String key = ((a->timerKey != nullptr) && (*a->timerKey != 0)) ? event->expandText(a->timerKey, alarm) : String();
               AddOneTimeScheduledTask(L"Execute.Action", time(nullptr) + timerDelay, parameters,
                        new ActionExecutionTransientData(event, alarm, m_guid), 0, event->getSourceId(), SYSTEM_ACCESS_FULL,
                        comments, key.isEmpty() ? nullptr : key.cstr(), true);
            }

            uint64_t snoozeTime = _tcstoul(event->expandText(a->snoozeTime, alarm).cstr(), nullptr, 10);
            if ((snoozeTime != 0) && (a->blockingTimerKey != nullptr) && (a->blockingTimerKey[0] != 0))
            {
               wchar_t parameters[128], comments[256];
               _sntprintf(parameters, 128, L"action=%u;event=" UINT64_FMT L";alarm=%u;rule=%s", a->actionId, event->getId(),
                     (alarm != nullptr) ? alarm->getAlarmId() : 0, m_guid.toString().cstr());
               _sntprintf(comments, 256, L"Snooze action execution for event %s", event->getName());
               String key = ((a->blockingTimerKey != nullptr) && (*a->blockingTimerKey != 0)) ? event->expandText(a->blockingTimerKey, alarm) : String();
               AddOneTimeScheduledTask(L"Dummy", time(nullptr) + snoozeTime, parameters,
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
            if (DeleteScheduledTasksByKey(key) > 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("Delayed action execution with key \"%s\" cancelled"), key.cstr());
            }
         }
      }
      delete alarm;
   }

   executeActionScript(event);

   // Update persistent storage if needed
   if (m_pstorageSetActions.size() > 0)
      m_pstorageSetActions.forEach(ExecutePstorageSetAction, event);
   for(int i = 0; i < m_pstorageDeleteActions.size(); i++)
   {
      String key = event->expandText(m_pstorageDeleteActions.get(i));
      if (!key.isEmpty())
         DeletePersistentStorageValue(key);
   }

   if (object != nullptr)
   {
      for(KeyValuePair<const TCHAR> *attribute : m_customAttributeSetActions)
      {
         String name = event->expandText(attribute->key);
         if (!name.isEmpty())
         {
            String value = event->expandText(attribute->value);
            object->setCustomAttribute(name, value, StateChange::IGNORE);
         }

      }
      for(int i = 0; i < m_customAttributeDeleteActions.size(); i++)
      {
         String name = event->expandText(m_customAttributeDeleteActions.get(i));
         if (!name.isEmpty())
         {
            object->deleteCustomAttribute(name);
         }
      }
   }

   // Update downtime
   if (m_flags & RF_START_DOWNTIME)
   {
      String tag(m_downtimeTag[0] != 0 ? m_downtimeTag : _T("default"));
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Requesting downtime \"%s\" start for object %s [%u]"), tag.cstr(), (object != nullptr) ? object->getName() : _T("(null)"), event->getSourceId());
      ThreadPoolExecuteSerialized(g_mainThreadPool, _T("DOWNTIME"), StartDowntime, event->getSourceId(), tag);
   }
   else if (m_flags & RF_END_DOWNTIME)
   {
      String tag(m_downtimeTag[0] != 0 ? m_downtimeTag : _T("default"));
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Requesting downtime \"%s\" end for object %s [%u]"), tag.cstr(), (object != nullptr) ? object->getName() : _T("(null)"), event->getSourceId());
      ThreadPoolExecuteSerialized(g_mainThreadPool, _T("DOWNTIME"), EndDowntime, event->getSourceId(), tag);
   }

   if ((m_aiAgentInstructions != nullptr) && (m_aiAgentInstructions[0] != 0))
   {
      ProcessEventWithAIAssistant(event, object, m_aiAgentInstructions);
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
                     (m_flags & RF_CREATE_TICKET) != 0, (m_flags & RF_REQUEST_AI_COMMENT) != 0);
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
   _sntprintf(szQuery, 256, _T("SELECT object_id FROM policy_source_list WHERE rule_id=%d and exclusion='0'"), m_id);
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

   // Load rule's sources exclusions
   _sntprintf(szQuery, 256, _T("SELECT object_id FROM policy_source_list WHERE rule_id=%d and exclusion='1'"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
         m_sourceExclusions.add(DBGetFieldULong(hResult, i, 0));
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

   // Load rule's events
   _sntprintf(szQuery, 256, _T("SELECT time_filter,date_filter FROM policy_time_frame_list WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         m_timeFrames.add(new TimeFrame(DBGetFieldULong(hResult, i, 0), DBGetFieldUInt64(hResult, i, 1)));
      }
      DBFreeResult(hResult);
   }
   else
   {
      bSuccess = false;
   }

   // Load rule's actions
   _sntprintf(szQuery, 256, _T("SELECT action_id,timer_delay,timer_key,blocking_timer_key,snooze_time,active FROM policy_action_list WHERE rule_id=%d"), m_id);
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
         bool active = DBGetFieldLong(hResult, i, 5) ? true : false;
         m_actions.add(new ActionExecutionConfiguration(actionId, timerDelay, snoozeTime, timerKey, blockingTimerKey, active));
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
         if (DBGetFieldLong(hResult, i, 1) == EPP_ACTION_SET)
         {
            m_pstorageSetActions.setPreallocated(_tcsdup(key), DBGetField(hResult, i, 2, nullptr, 0));
         }
         if (DBGetFieldLong(hResult, i, 1) == EPP_ACTION_DELETE)
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

   // Load custom attributes actions
   _sntprintf(szQuery, 256, _T("SELECT attribute_name,action,value FROM policy_cattr_actions WHERE rule_id=%d"), m_id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != nullptr)
   {
      TCHAR key[MAX_DB_STRING];
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, key, MAX_DB_STRING);
         if (DBGetFieldLong(hResult, i, 1) == EPP_ACTION_SET)
         {
            m_customAttributeSetActions.setPreallocated(_tcsdup(key), DBGetField(hResult, i, 2, nullptr, 0));
         }
         if (DBGetFieldLong(hResult, i, 1) == EPP_ACTION_DELETE)
         {
            m_customAttributeDeleteActions.add(key);
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
static EnumerationCallbackResult SaveEppActions(const TCHAR *key, const void *value, void *data)
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
	TCHAR pszQuery[1024];
   // General attributes
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_impact,")
                                  _T("alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event,rca_script_name,")
                                  _T("action_script,downtime_tag,ai_instructions) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_guid);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_flags);
      DBBind(hStmt, 4, DB_SQLTYPE_TEXT,  m_comments, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_alarmMessage, DB_BIND_STATIC, MAX_EVENT_MSG_LENGTH);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_alarmImpact, DB_BIND_STATIC, 1000);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_alarmSeverity);
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_alarmKey, DB_BIND_STATIC, MAX_DB_STRING);
      DBBind(hStmt, 9, DB_SQLTYPE_TEXT, m_filterScriptSource, DB_BIND_STATIC);
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_alarmTimeout);
      DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_alarmTimeoutEvent);
      DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_rcaScriptName, DB_BIND_STATIC, MAX_DB_STRING);
      DBBind(hStmt, 13, DB_SQLTYPE_TEXT, m_actionScriptSource, DB_BIND_STATIC);
      DBBind(hStmt, 14, DB_SQLTYPE_VARCHAR, m_downtimeTag, DB_BIND_STATIC);
      DBBind(hStmt, 15, DB_SQLTYPE_TEXT, m_aiAgentInstructions, DB_BIND_STATIC);
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
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO policy_action_list (rule_id,action_id,timer_delay,timer_key,blocking_timer_key,snooze_time,active,record_id) VALUES (?,?,?,?,?,?,?,?)"), m_actions.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; i < m_actions.size() && success; i++)
         {
            const ActionExecutionConfiguration *a = m_actions.get(i);
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, a->actionId);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, a->timerDelay, DB_BIND_STATIC, 127);
            DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, a->timerKey, DB_BIND_STATIC, 127);
            DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, a->blockingTimerKey, DB_BIND_STATIC, 127);
            DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, a->snoozeTime, DB_BIND_STATIC, 127);
            DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, a->active ? _T("1") : _T("0"), DB_BIND_STATIC);
            DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, i);
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
         for(int i = 0; i < m_timerCancellations.size() && success; i++)
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
      for(int i = 0; i < m_events.size() && success; i++)
      {
         _sntprintf(pszQuery, 1024, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), m_id, m_events.get(i));
         success = DBQuery(hdb, pszQuery);
      }
   }

   // Time frames
   if (success && !m_timeFrames.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO policy_time_frame_list (rule_id,time_frame_id,time_filter,date_filter) VALUES (?,?,?,?)"), m_timeFrames.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         int i = 0;
         for(TimeFrame *frame : m_timeFrames)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i++);
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, frame->getTimeFilter());
            DBBind(hStmt, 4, DB_SQLTYPE_BIGINT, frame->getDateFilter());
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Sources
   if (success && !m_sources.isEmpty())
   {
      for(int i = 0; i < m_sources.size() && success; i++)
      {
         _sntprintf(pszQuery, 1024, _T("INSERT INTO policy_source_list (rule_id,object_id,exclusion) VALUES (%d,%d,'0')"), m_id, m_sources.get(i));
         success = DBQuery(hdb, pszQuery);
      }
   }

   // Source exclusions
   if (success && !m_sourceExclusions.isEmpty())
   {
      for(int i = 0; i < m_sourceExclusions.size() && success; i++)
      {
         _sntprintf(pszQuery, 1024, _T("INSERT INTO policy_source_list (rule_id,object_id,exclusion) VALUES (%d,%d,'1')"), m_id, m_sourceExclusions.get(i));
         success = DBQuery(hdb, pszQuery);
      }
   }

	// Persistent storage actions
   if (success && !m_pstorageSetActions.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO policy_pstorage_actions (rule_id,action,ps_key,value) VALUES (?,'1',?,?)"), m_pstorageSetActions.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         success = _STOP != m_pstorageSetActions.forEach(SaveEppActions, hStmt);
         DBFreeStatement(hStmt);
      }
   }

   if (success && !m_pstorageDeleteActions.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO policy_pstorage_actions (rule_id,action,ps_key) VALUES (?,'2',?)"), m_pstorageDeleteActions.size() > 1);
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

   // Custom attribute actions
   if (success && !m_customAttributeSetActions.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO policy_cattr_actions (rule_id,action,attribute_name,value) VALUES (?,'1',?,?)"), m_customAttributeSetActions.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         success = _STOP != m_customAttributeSetActions.forEach(SaveEppActions, hStmt);
         DBFreeStatement(hStmt);
      }
   }

   if (success && !m_customAttributeDeleteActions.isEmpty())
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO policy_cattr_actions (rule_id,action,attribute_name) VALUES (?,'2',?)"), m_pstorageDeleteActions.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; i < m_customAttributeDeleteActions.size() && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_customAttributeDeleteActions.get(i), DB_BIND_STATIC, 127);
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
         for(int i = 0; (i < m_alarmCategoryList.size()) && success; i++)
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
void EPRule::fillMessage(NXCPMessage *msg) const
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
   msg->setField(VID_DOWNTIME_TAG, m_downtimeTag);
   msg->setField(VID_COMMENTS, CHECK_NULL_EX(m_comments));
   msg->setField(VID_AI_AGENT_INSTRUCTIONS, CHECK_NULL_EX(m_aiAgentInstructions));
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
      msg->setField(fieldId++, a->active);
      fieldId += 4;
   }
   msg->setField(VID_TIMER_LIST, m_timerCancellations);
   msg->setFieldFromInt32Array(VID_RULE_EVENTS, &m_events);
   msg->setFieldFromInt32Array(VID_RULE_SOURCES, &m_sources);
   msg->setFieldFromInt32Array(VID_RULE_SOURCE_EXCLUSIONS, &m_sourceExclusions);

   msg->setField(VID_NUM_TIME_FRAMES, static_cast<uint32_t>(m_timeFrames.size()));
   fieldId = VID_TIME_FRAME_LIST_BASE;
   for(int i = 0; i < m_timeFrames.size(); i++)
   {
      TimeFrame *timeFrame = m_timeFrames.get(i);
      msg->setField(fieldId++, timeFrame->getTimeFilter());
      msg->setField(fieldId++, timeFrame->getDateFilter());
   }
   msg->setField(VID_SCRIPT, CHECK_NULL_EX(m_filterScriptSource));
   msg->setField(VID_ACTION_SCRIPT, CHECK_NULL_EX(m_actionScriptSource));
   m_pstorageSetActions.fillMessage(msg, VID_PSTORAGE_SET_LIST_BASE, VID_NUM_SET_PSTORAGE);
   m_pstorageDeleteActions.fillMessage(msg, VID_PSTORAGE_DELETE_LIST_BASE, VID_NUM_DELETE_PSTORAGE);
   m_customAttributeSetActions.fillMessage(msg, VID_CUSTOM_ATTR_SET_LIST_BASE, VID_CUSTOM_ATTR_SET_COUNT);
   m_customAttributeDeleteActions.fillMessage(msg, VID_CUSTOM_ATTR_DEL_LIST_BASE, VID_CUSTOM_ATTR_DEL_COUNT);
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
   json_object_set_new(root, "sourceExclusions", m_sourceExclusions.toJson());
   json_object_set_new(root, "events", m_events.toJson());
   json_t *timeFrames = json_array();
   for(TimeFrame *frame : m_timeFrames)
   {
      json_t *timeFrame = json_object();
      json_object_set_new(timeFrame, "time", json_integer(frame->getTimeFilter()));
      json_object_set_new(timeFrame, "date", json_integer(frame->getDateFilter()));
      json_array_append_new(timeFrames, timeFrame);
   }
   json_object_set_new(root, "timeFrames", timeFrames);
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
      json_object_set_new(action, "active", json_boolean(d->active));
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
   json_object_set_new(root, "filterScript", json_string_t(m_filterScriptSource));
   json_object_set_new(root, "actionScript", json_string_t(m_actionScriptSource));
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
   json_object_set_new(root, "customAttributeSetActions", m_customAttributeSetActions.toJson());
   json_object_set_new(root, "customAttributeDeleteActions", m_customAttributeDeleteActions.toJson());
   json_object_set_new(root, "aiAgentInstructions", json_string_t(m_aiAgentInstructions));

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
   hResult = DBSelect(hdb, L"SELECT rule_id,rule_guid,flags,comments,alarm_message,"
                           L"alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event,"
                           L"rca_script_name,alarm_impact,action_script,downtime_tag,ai_instructions "
                           L"FROM event_policy ORDER BY rule_id");
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
      success = DBQuery(hdb, L"DELETE FROM event_policy") &&
                DBQuery(hdb, L"DELETE FROM policy_time_frame_list") &&
                DBQuery(hdb, L"DELETE FROM policy_action_list") &&
                DBQuery(hdb, L"DELETE FROM policy_timer_cancellation_list") &&
                DBQuery(hdb, L"DELETE FROM policy_event_list") &&
                DBQuery(hdb, L"DELETE FROM policy_source_list") &&
                DBQuery(hdb, L"DELETE FROM policy_pstorage_actions") &&
                DBQuery(hdb, L"DELETE FROM policy_cattr_actions") &&
                DBQuery(hdb, L"DELETE FROM alarm_category_map");

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
	nxlog_debug_tag(DEBUG_TAG, 7, L"EPP: processing event " UINT64_FMT, pEvent->getId());
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
      m_rules.get(i)->fillMessage(&msg);
      session->sendMessage(msg);
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
 * Validate event policy configuration
 */
void EventPolicy::validateConfig() const
{
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      m_rules.get(i)->validateConfig();
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
void EventPolicy::exportRule(TextFileWriter& xml, const uuid& guid) const
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
void EventPolicy::exportRuleOrgering(TextFileWriter& xml) const
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
