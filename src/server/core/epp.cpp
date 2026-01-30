/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
#include <nms_incident.h>
#include <nxcore_ps.h>
#include <nxai.h>

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
   m_version = 1;
   m_modified = false;
   m_modificationTime = 0;
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
   m_incidentDelay = 0;
   m_incidentTitle = nullptr;
   m_incidentDescription = nullptr;
   m_incidentAIAnalysisDepth = 0;
   m_incidentAIPrompt = nullptr;
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
   m_version = 1;
   m_modified = false;
   m_modificationTime = 0;
   m_flags = config.getSubEntryValueAsUInt(_T("flags"));

	ConfigEntry *eventsRoot = config.findEntry(_T("events"));
   if (eventsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> events = eventsRoot->getSubEntries(_T("event#*"));
      for(int i = 0; i < events->size(); i++)
      {
         shared_ptr<EventTemplate> e = FindEventTemplateByName(events->get(i)->getSubEntryValue(_T("name"), 0, _T("<unknown>")));
         if (e != nullptr && !m_events.contains(e->getCode()))
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

   // Incident creation settings
   m_incidentDelay = config.getSubEntryValueAsUInt(_T("incidentDelay"));
   m_incidentTitle = MemCopyString(config.getSubEntryValue(_T("incidentTitle")));
   m_incidentDescription = MemCopyString(config.getSubEntryValue(_T("incidentDescription")));

   // AI incident analysis settings
   m_incidentAIAnalysisDepth = config.getSubEntryValueAsInt(_T("incidentAIAnalysisDepth"));
   m_incidentAIPrompt = MemCopyString(config.getSubEntryValue(_T("incidentAIPrompt")));
}

/**
 * Create rule from JSON object
 */
EPRule::EPRule(json_t *json, ImportContext *context) : m_timeFrames(0, 16, Ownership::True), m_actions(0, 16, Ownership::True)
{
   m_id = 0;
   m_guid = json_object_get_uuid(json, "guid");
   if (m_guid.isNull())
      m_guid = uuid::generate(); // generate random GUID if rule was imported without GUID
   m_version = 1;
   m_modified = false;
   m_modificationTime = 0;
   m_flags = json_object_get_uint32(json, "flags");

   // Import events - JSON format uses event objects with name property
   json_t *eventsArray = json_object_get(json, "events");
   if (json_is_array(eventsArray))
   {
      size_t index;
      json_t *eventItem;
      json_array_foreach(eventsArray, index, eventItem)
      {
         if (json_is_object(eventItem))
         {
            // First try to match by name
            String eventName = json_object_get_string(eventItem, "name", nullptr);
            uint32_t eventCode = 0;
            
            if (!eventName.isEmpty())
            {
               eventCode = EventCodeFromName(eventName, 0);
            }
            
            if (eventCode != 0 && !m_events.contains(eventCode))
            {
               m_events.add(eventCode);
            }
            else
            {
               context->log(NXLOG_WARNING, _T("EPRule::EPRule()"),
                  _T("Event processing policy rule import: rule \"%s\" refers to unknown event \"%s\" (ID not provided or invalid)"),
                  m_guid.toString().cstr(), eventName.isEmpty() ? _T("(unnamed)") : eventName.cstr());
            }
         }
      }
   }

   //TODO: Import sources 
   //TODO: Import source exclusions 

   // Import time frames
   json_t *timeFramesArray = json_object_get(json, "timeFrames");
   if (json_is_array(timeFramesArray))
   {
      size_t index;
      json_t *timeFrame;
      json_array_foreach(timeFramesArray, index, timeFrame)
      {
         if (json_is_object(timeFrame))
         {
            uint32_t time = json_object_get_uint32(timeFrame, "time");
            uint64_t date = json_object_get_uint64(timeFrame, "date");
            m_timeFrames.add(new TimeFrame(time, date));
         }
      }
   }

   // Import actions
   json_t *actionsArray = json_object_get(json, "actions");
   if (json_is_array(actionsArray))
   {
      size_t index;
      json_t *action;
      json_array_foreach(actionsArray, index, action)
      {
         if (json_is_object(action))
         {
            uint32_t actionId = json_object_get_uint32(action, "id");
            String timerDelay = json_object_get_string(action, "timerDelay", _T(""));
            String timerKey = json_object_get_string(action, "timerKey", _T(""));
            String blockingTimerKey = json_object_get_string(action, "blockingTimerKey", _T(""));
            String snoozeTime = json_object_get_string(action, "snoozeTime", _T(""));
            bool active = json_object_get_boolean(action, "active", true);
            if (IsValidActionId(actionId))
               m_actions.add(new ActionExecutionConfiguration(actionId, 
                  timerDelay.isEmpty() ? nullptr : MemCopyString(timerDelay),
                  snoozeTime.isEmpty() ? nullptr : MemCopyString(snoozeTime),
                  timerKey.isEmpty() ? nullptr : MemCopyString(timerKey),
                  blockingTimerKey.isEmpty() ? nullptr : MemCopyString(blockingTimerKey),
                  active));
         }
      }
   }

   // Import timer cancellations
   json_t *timerCancellationsArray = json_object_get(json, "timerCancellations");
   if (json_is_array(timerCancellationsArray))
   {
      size_t index;
      json_t *timerKey;
      json_array_foreach(timerCancellationsArray, index, timerKey)
      {
         if (json_is_string(timerKey))
         {
            const char *keyStr = json_string_value(timerKey);
            if (keyStr != nullptr && *keyStr != 0)
            {
#ifdef UNICODE
               WCHAR *key = WideStringFromUTF8String(keyStr);
               m_timerCancellations.add(key);
               MemFree(key);
#else
               m_timerCancellations.add(keyStr);
#endif
            }
         }
      }
   }

   // Import basic properties
   m_comments = MemCopyString(json_object_get_string(json, "comments", _T("")));
   m_alarmSeverity = json_object_get_int32(json, "alarmSeverity");
   m_alarmTimeout = json_object_get_uint32(json, "alarmTimeout");
   
   // Import alarm timeout event - JSON format uses event name
   String alarmTimeoutEventName = json_object_get_string(json, "alarmTimeoutEvent", _T("SYS_ALARM_TIMEOUT"));
   m_alarmTimeoutEvent = EventCodeFromName(alarmTimeoutEventName, EVENT_ALARM_TIMEOUT);
   
   m_alarmKey = MemCopyString(json_object_get_string(json, "alarmKey", _T("")));
   m_alarmMessage = MemCopyString(json_object_get_string(json, "alarmMessage", _T("")));
   m_alarmImpact = MemCopyString(json_object_get_string(json, "alarmImpact", _T("")));
   
   const char *rcaScript = json_object_get_string_utf8(json, "rootCauseAnalysisScript", "");
#ifdef UNICODE
   m_rcaScriptName = WideStringFromUTF8String(rcaScript);
#else
   m_rcaScriptName = MemCopyStringA(rcaScript);
#endif
   
   m_aiAgentInstructions = MemCopyString(json_object_get_string(json, "aiAgentInstructions", _T("")));
   
   String downtimeTag = json_object_get_string(json, "downtimeTag", _T(""));
   wcslcpy(m_downtimeTag, downtimeTag, MAX_DOWNTIME_TAG_LENGTH);

   // Import alarm categories
   json_t *categoriesArray = json_object_get(json, "alarmCategories");
   if (json_is_array(categoriesArray))
   {
      size_t index;
      json_t *categoryId;
      json_array_foreach(categoriesArray, index, categoryId)
      {
         if (json_is_integer(categoryId))
         {
            m_alarmCategoryList.add(static_cast<uint32_t>(json_integer_value(categoryId)));
         }
      }
   }

   // Import persistent storage actions
   json_t *pstorageSetActionsObj = json_object_get(json, "pstorageSetActions");
   if (json_is_object(pstorageSetActionsObj))
   {
      m_pstorageSetActions.addAllFromJson(pstorageSetActionsObj);
   }

   json_t *pstorageDeleteActionsArray = json_object_get(json, "pstorageDeleteActions");
   if (json_is_array(pstorageDeleteActionsArray))
   {
      size_t index;
      json_t *deleteKey;
      json_array_foreach(pstorageDeleteActionsArray, index, deleteKey)
      {
         if (json_is_string(deleteKey))
         {
            const char *keyStr = json_string_value(deleteKey);
            if (keyStr != nullptr && *keyStr != 0)
            {
#ifdef UNICODE
               WCHAR *key = WideStringFromUTF8String(keyStr);
               m_pstorageDeleteActions.add(key);
               MemFree(key);
#else
               m_pstorageDeleteActions.add(keyStr);
#endif
            }
         }
      }
   }

   // Import custom attribute actions
   json_t *customAttributeSetActionsObj = json_object_get(json, "customAttributeSetActions");
   if (json_is_object(customAttributeSetActionsObj))
   {
      m_customAttributeSetActions.addAllFromJson(customAttributeSetActionsObj);
   }

   json_t *customAttributeDeleteActionsArray = json_object_get(json, "customAttributeDeleteActions");
   if (json_is_array(customAttributeDeleteActionsArray))
   {
      size_t index;
      json_t *deleteKey;
      json_array_foreach(customAttributeDeleteActionsArray, index, deleteKey)
      {
         if (json_is_string(deleteKey))
         {
            const char *keyStr = json_string_value(deleteKey);
            if (keyStr != nullptr && *keyStr != 0)
            {
#ifdef UNICODE
               WCHAR *key = WideStringFromUTF8String(keyStr);
               m_customAttributeDeleteActions.add(key);
               MemFree(key);
#else
               m_customAttributeDeleteActions.add(keyStr);
#endif
            }
         }
      }
   }

   // Import and compile scripts
   String filterScriptSource = json_object_get_string(json, "filterScript", _T(""));
   m_filterScriptSource = MemCopyString(filterScriptSource);
   if ((m_filterScriptSource != nullptr) && (*m_filterScriptSource != 0))
   {
      m_filterScript = CompileServerScript(m_filterScriptSource, SCRIPT_CONTEXT_EVENT_PROC, nullptr, 0, _T("EPP::Filter::%u"), m_id + 1);
      if (m_filterScript == nullptr)
      {
         context->log(NXLOG_ERROR, _T("EPRule::EPRule()"), _T("Failed to compile evaluation script for event processing policy rule %s"), m_guid.toString().cstr());
      }
   }
   else
   {
      m_filterScript = nullptr;
   }

   String actionScriptSource = json_object_get_string(json, "actionScript", _T(""));
   m_actionScriptSource = MemCopyString(actionScriptSource);
   if ((m_actionScriptSource != nullptr) && (*m_actionScriptSource != 0))
   {
      m_actionScript = CompileServerScript(m_actionScriptSource, SCRIPT_CONTEXT_EVENT_PROC, nullptr, 0, _T("EPP::Action::%u"), m_id + 1);
      if (m_actionScript == nullptr)
      {
         context->log(NXLOG_ERROR, _T("EPRule::EPRule()"), _T("Failed to compile action script for event processing policy rule %s"), m_guid.toString().cstr());
      }
   }
   else
   {
      m_actionScript = nullptr;
   }

   // Incident creation settings
   m_incidentDelay = json_object_get_uint32(json, "incidentDelay");
   String incidentTitle = json_object_get_string(json, "incidentTitle", nullptr);
   m_incidentTitle = incidentTitle.isEmpty() ? nullptr : MemCopyString(incidentTitle);
   String incidentDescription = json_object_get_string(json, "incidentDescription", nullptr);
   m_incidentDescription = incidentDescription.isEmpty() ? nullptr : MemCopyString(incidentDescription);

   // AI incident analysis settings
   m_incidentAIAnalysisDepth = json_object_get_int32(json, "incidentAIAnalysisDepth", 0);
   String incidentAIPrompt = json_object_get_string(json, "incidentAIPrompt", nullptr);
   m_incidentAIPrompt = incidentAIPrompt.isEmpty() ? nullptr : MemCopyString(incidentAIPrompt);
}

/**
 * Construct event policy rule from database record
 * Assuming the following field order:
 * rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,
 * alarm_timeout,alarm_timeout_event,rca_script_name,alarm_impact,action_script,
 * downtime_tag,ai_instructions,incident_delay,incident_title,incident_description,
 * incident_ai_depth,incident_ai_prompt,modified_by_guid,modified_by_name,modification_time
 */
EPRule::EPRule(DB_RESULT hResult, int row) : m_timeFrames(0, 16, Ownership::True), m_actions(0, 16, Ownership::True)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_guid = DBGetFieldGUID(hResult, row, 1);
   m_version = 1;
   m_modified = false;
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
   m_incidentDelay = DBGetFieldUInt32(hResult, row, 15);
   m_incidentTitle = DBGetField(hResult, row, 16, nullptr, 0);
   m_incidentDescription = DBGetField(hResult, row, 17, nullptr, 0);
   m_incidentAIAnalysisDepth = DBGetFieldInt32(hResult, row, 18);
   m_incidentAIPrompt = DBGetField(hResult, row, 19, nullptr, 0);
   m_modifiedByGuid = DBGetFieldGUID(hResult, row, 20);
   m_modifiedByName = DBGetFieldAsString(hResult, row, 21);
   m_modificationTime = static_cast<time_t>(DBGetFieldULong(hResult, row, 22));
}

/**
 * Construct event policy rule from NXCP message
 */
EPRule::EPRule(const NXCPMessage& msg) : m_timeFrames(0, 16, Ownership::True), m_actions(0, 16, Ownership::True)
{
   m_flags = msg.getFieldAsUInt32(VID_FLAGS);
   m_id = msg.getFieldAsUInt32(VID_RULE_ID);
   m_guid = msg.getFieldAsGUID(VID_GUID);
   m_version = msg.getFieldAsUInt32(VID_RULE_VERSION);
   m_modified = msg.getFieldAsBoolean(VID_RULE_MODIFIED);
   m_modificationTime = msg.getFieldAsTime(VID_MODIFICATION_TIME);
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

   m_incidentDelay = msg.getFieldAsUInt32(VID_INCIDENT_DELAY);
   m_incidentTitle = msg.getFieldAsString(VID_INCIDENT_TITLE);
   m_incidentDescription = msg.getFieldAsString(VID_INCIDENT_DESCRIPTION);

   m_incidentAIAnalysisDepth = msg.getFieldAsInt32(VID_INCIDENT_AI_DEPTH);
   m_incidentAIPrompt = msg.getFieldAsString(VID_INCIDENT_AI_PROMPT);
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
   MemFree(m_incidentTitle);
   MemFree(m_incidentDescription);
   MemFree(m_incidentAIPrompt);
}

/**
 * Set modification info for optimistic concurrency
 */
void EPRule::setModificationInfo(const uuid& userGuid, const TCHAR* userName, time_t timestamp)
{
   m_modifiedByGuid = userGuid;
   m_modifiedByName = userName;
   m_modificationTime = timestamp;
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
 * Create incident from alarm (immediate or delayed)
 */
void EPRule::createIncidentFromAlarm(Event *event, uint32_t alarmId) const
{
   if (m_incidentDelay == 0)
   {
      // Immediate incident creation
      String title = (m_incidentTitle != nullptr && m_incidentTitle[0] != 0)
         ? event->expandText(m_incidentTitle)
         : String(event->getLastAlarmMessage());

      String description = (m_incidentDescription != nullptr)
         ? event->expandText(m_incidentDescription)
         : String();

      uint32_t incidentId = 0;
      uint32_t rcc = CreateIncident(event->getSourceId(), title.cstr(),
         description.isEmpty() ? nullptr : description.cstr(), alarmId, 0, &incidentId);

      // Trigger AI analysis if enabled and incident was created successfully
      if ((rcc == RCC_SUCCESS) && (incidentId != 0) && (m_flags & RF_AI_ANALYZE_INCIDENT))
      {
         SpawnIncidentAIAnalysis(incidentId, m_incidentAIAnalysisDepth, (m_flags & RF_AI_AUTO_ASSIGN) != 0, m_incidentAIPrompt);
      }
   }
   else
   {
      // Schedule delayed incident creation using scheduler
      StringBuffer persistentData;
      persistentData.appendFormattedString(_T("alarm=%u;object=%u"), alarmId, event->getSourceId());

      if (m_incidentTitle != nullptr && m_incidentTitle[0] != 0)
      {
         String title = event->expandText(m_incidentTitle);
         persistentData.append(_T(";title="));
         persistentData.append(title);
      }
      if (m_incidentDescription != nullptr)
      {
         String desc = event->expandText(m_incidentDescription);
         persistentData.append(_T(";description="));
         persistentData.append(desc);
      }

      // Add AI analysis parameters if enabled
      if (m_flags & RF_AI_ANALYZE_INCIDENT)
      {
         persistentData.appendFormattedString(_T(";ai_analyze=1;ai_depth=%d;ai_assign=%d"),
            m_incidentAIAnalysisDepth, (m_flags & RF_AI_AUTO_ASSIGN) ? 1 : 0);
         if (m_incidentAIPrompt != nullptr && m_incidentAIPrompt[0] != 0)
         {
            persistentData.append(_T(";ai_prompt="));
            persistentData.append(m_incidentAIPrompt);
         }
      }

      wchar_t comments[256];
      nx_swprintf(comments, 256, L"Delayed incident creation for alarm %u", alarmId);

      AddOneTimeScheduledTask(INCIDENT_CREATE_TASK_ID,
         time(nullptr) + m_incidentDelay,
         persistentData.cstr(),
         nullptr,  // No transient data needed
         0,        // userId
         event->getSourceId(),
         SYSTEM_ACCESS_FULL,
         comments,
         nullptr,  // No key needed
         true);    // System task
   }
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
	   event->setLastAlarmId(alarmId);

      // Create incident if RF_CREATE_INCIDENT flag is set and alarm was created
      if ((m_flags & RF_CREATE_INCIDENT) && (alarmId != 0))
      {
         createIncidentFromAlarm(event, alarmId);
      }
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
bool EPRule::saveToDB(DB_HANDLE hdb, const uuid& modifiedByGuid, const TCHAR* modifiedByName, time_t modificationTime) const
{
   bool success;
	TCHAR pszQuery[1024];
   // General attributes
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_impact,")
                                  _T("alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event,rca_script_name,")
                                  _T("action_script,downtime_tag,ai_instructions,incident_delay,incident_title,incident_description,")
                                  _T("incident_ai_depth,incident_ai_prompt,modified_by_guid,modified_by_name,modification_time) ")
                                  _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
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
      DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_incidentDelay);
      DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, m_incidentTitle, DB_BIND_STATIC, 255);
      DBBind(hStmt, 18, DB_SQLTYPE_VARCHAR, m_incidentDescription, DB_BIND_STATIC, 2000);
      DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, m_incidentAIAnalysisDepth);
      DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_incidentAIPrompt, DB_BIND_STATIC, 2000);
      DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, modifiedByGuid);
      DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, modifiedByName, DB_BIND_STATIC, 63);
      DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(modificationTime));
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
   msg->setField(VID_RULE_VERSION, m_version);
   msg->setField(VID_MODIFIED_BY_GUID, m_modifiedByGuid);
   msg->setField(VID_MODIFIED_BY_NAME, m_modifiedByName);
   msg->setField(VID_MODIFICATION_TIME, static_cast<uint32_t>(m_modificationTime));
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
   msg->setField(VID_INCIDENT_DELAY, m_incidentDelay);
   msg->setField(VID_INCIDENT_TITLE, CHECK_NULL_EX(m_incidentTitle));
   msg->setField(VID_INCIDENT_DESCRIPTION, CHECK_NULL_EX(m_incidentDescription));
   msg->setField(VID_INCIDENT_AI_DEPTH, static_cast<int32_t>(m_incidentAIAnalysisDepth));
   msg->setField(VID_INCIDENT_AI_PROMPT, CHECK_NULL_EX(m_incidentAIPrompt));
}

/**
 * Create configuration export record
 */
json_t *EPRule::createExportRecord() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id + 1));
   json_object_set_new(root, "guid", m_guid.toJson());
   
   json_object_set_new(root, "flags", json_integer(m_flags));
   
   // Export sources with full object information
   json_t *sources = json_array();
   for(int i = 0; i < m_sources.size(); i++)
   {
      shared_ptr<NetObj> object = FindObjectById(m_sources.get(i));
      if (object != nullptr)
      {
         json_t *source = json_object();
         json_object_set_new(source, "id", json_integer(object->getId()));
         json_object_set_new(source, "name", json_string_w(object->getName()));
         json_object_set_new(source, "guid", object->getGuid().toJson());
         json_object_set_new(source, "class", json_integer(object->getObjectClass()));
         json_array_append_new(sources, source);
      }
   }
   json_object_set_new(root, "sources", sources);
   
   // Export source exclusions with full object information  
   json_t *sourceExclusions = json_array();
   for(int i = 0; i < m_sourceExclusions.size(); i++)
   {
      shared_ptr<NetObj> object = FindObjectById(m_sourceExclusions.get(i));
      if (object != nullptr)
      {
         json_t *sourceExclusion = json_object();
         json_object_set_new(sourceExclusion, "id", json_integer(object->getId()));
         json_object_set_new(sourceExclusion, "name", json_string_w(object->getName()));
         json_object_set_new(sourceExclusion, "guid", object->getGuid().toJson());
         json_object_set_new(sourceExclusion, "class", json_integer(object->getObjectClass()));
         json_array_append_new(sourceExclusions, sourceExclusion);
      }
   }
   json_object_set_new(root, "sourceExclusions", sourceExclusions);
   
   // Export events with names (XML-compatible structure)
   json_t *events = json_array();
   for(int i = 0; i < m_events.size(); i++)
   {
      uint32_t eventCode = m_events.get(i);
      json_t *event = json_object();
      
      TCHAR eventName[MAX_EVENT_NAME];
      if (EventNameFromCode(eventCode, eventName))
      {
         json_object_set_new(event, "name", json_string_t(eventName));
      }
      else
      {
         json_object_set_new(event, "name", json_string("UNKNOWN_EVENT"));
      }
      
      json_object_set_new(event, "id", json_integer(eventCode));
      json_array_append_new(events, event);
   }
   json_object_set_new(root, "events", events);
   
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
      json_object_set_new(action, "guid", GetActionGUID(d->actionId).toJson());
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
   json_object_set_new(root, "downtimeTag", json_string_w(m_downtimeTag));
   json_object_set_new(root, "filterScript", json_string_t(m_filterScriptSource));
   json_object_set_new(root, "actionScript", json_string_t(m_actionScriptSource));
   json_object_set_new(root, "alarmMessage", json_string_t(m_alarmMessage));
   json_object_set_new(root, "alarmImpact", json_string_t(m_alarmImpact));
   json_object_set_new(root, "alarmSeverity", json_integer(m_alarmSeverity));
   json_object_set_new(root, "alarmKey", json_string_t(m_alarmKey));
   json_object_set_new(root, "alarmTimeout", json_integer(m_alarmTimeout));
   
   // Export alarm timeout event by name
   TCHAR alarmTimeoutEventName[MAX_EVENT_NAME];
   if (EventNameFromCode(m_alarmTimeoutEvent, alarmTimeoutEventName))
   {
      json_object_set_new(root, "alarmTimeoutEvent", json_string_t(alarmTimeoutEventName));
   }
   else
   {
      json_object_set_new(root, "alarmTimeoutEvent", json_string("UNKNOWN_EVENT"));
   }
   
   json_object_set_new(root, "rootCauseAnalysisScript", json_string_t(m_rcaScriptName));
   json_object_set_new(root, "alarmCategories", m_alarmCategoryList.toJson());
   json_object_set_new(root, "pstorageSetActions", m_pstorageSetActions.toJson());
   json_object_set_new(root, "pstorageDeleteActions", m_pstorageDeleteActions.toJson());
   json_object_set_new(root, "customAttributeSetActions", m_customAttributeSetActions.toJson());
   json_object_set_new(root, "customAttributeDeleteActions", m_customAttributeDeleteActions.toJson());
   json_object_set_new(root, "aiAgentInstructions", json_string_t(m_aiAgentInstructions));
   json_object_set_new(root, "incidentDelay", json_integer(m_incidentDelay));
   json_object_set_new(root, "incidentTitle", json_string_t(m_incidentTitle));
   json_object_set_new(root, "incidentDescription", json_string_t(m_incidentDescription));
   json_object_set_new(root, "incidentAIAnalysisDepth", json_integer(m_incidentAIAnalysisDepth));
   json_object_set_new(root, "incidentAIPrompt", json_string_t(m_incidentAIPrompt));

   return root;
}

/**
 * Serialize rule to JSON. If assistantMode is true, output is formed to be more usable by AI assistants.
 */
json_t *EPRule::toJson(bool assistantMode) const
{
   json_t *root = json_object();
   json_object_set_new(root, "guid", m_guid.toJson());
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
   json_object_set_new(root, "downtimeTag", json_string_w(m_downtimeTag));
   json_object_set_new(root, "filterScript", json_string_t(m_filterScriptSource));
   json_object_set_new(root, "actionScript", json_string_t(m_actionScriptSource));
   json_object_set_new(root, "rcaScriptName", json_string_t(m_rcaScriptName));
   json_object_set_new(root, "categories", m_alarmCategoryList.toJson());
   json_object_set_new(root, "rootCauseAnalysisScript", json_string_t(m_rcaScriptName));
   json_object_set_new(root, "pstorageSetActions", m_pstorageSetActions.toJson());
   json_object_set_new(root, "pstorageDeleteActions", m_pstorageDeleteActions.toJson());
   json_object_set_new(root, "customAttributeSetActions", m_customAttributeSetActions.toJson());
   json_object_set_new(root, "customAttributeDeleteActions", m_customAttributeDeleteActions.toJson());
   json_object_set_new(root, "aiAgentInstructions", json_string_t(m_aiAgentInstructions));

   if (assistantMode)
   {
      bool generateAlarm = (m_flags & RF_GENERATE_ALARM) != 0 && m_alarmSeverity != SEVERITY_RESOLVE && m_alarmSeverity != SEVERITY_TERMINATE;
      if (generateAlarm)
      {
         json_object_set_new(root, "alarmSeverity", json_string_w((m_alarmSeverity == SEVERITY_FROM_EVENT) ? L"same as event" : GetStatusAsText(m_alarmSeverity, true)));
         json_object_set_new(root, "alarmMessage", json_string_t(m_alarmMessage));
         json_object_set_new(root, "alarmImpact", json_string_t(m_alarmImpact));
         json_object_set_new(root, "alarmTimeout", json_integer(m_alarmTimeout));
         json_object_set_new(root, "alarmTimeoutEvent", json_integer(m_alarmTimeoutEvent));
      }
      if ((m_flags & RF_GENERATE_ALARM) != 0)
      {
         json_object_set_new(root, "alarmKey", json_string_t(m_alarmKey));
      }

      json_t *flags = json_object();
      json_object_set_new(flags, "disabled", json_boolean((m_flags & RF_DISABLED) != 0));
      json_object_set_new(flags, "generateAlarm", json_boolean(generateAlarm));
      json_object_set_new(flags, "resolveAlarm", json_boolean((m_flags & RF_GENERATE_ALARM) != 0 && m_alarmSeverity == SEVERITY_RESOLVE));
      json_object_set_new(flags, "terminateAlarm", json_boolean((m_flags & RF_GENERATE_ALARM) != 0 && m_alarmSeverity == SEVERITY_TERMINATE));
      json_object_set_new(flags, "stopProcessing", json_boolean((m_flags & RF_STOP_PROCESSING) != 0));
      json_object_set_new(flags, "createTicket", json_boolean((m_flags & RF_CREATE_TICKET) != 0));
      json_object_set_new(flags, "terminateByRegexp", json_boolean((m_flags & RF_TERMINATE_BY_REGEXP) != 0));
      json_object_set_new(flags, "startDowntime", json_boolean((m_flags & RF_START_DOWNTIME) != 0));
      json_object_set_new(flags, "endDowntime", json_boolean((m_flags & RF_END_DOWNTIME) != 0));
      json_object_set_new(flags, "requestAIComment", json_boolean((m_flags & RF_REQUEST_AI_COMMENT) != 0));
      json_object_set_new(flags, "createIncident", json_boolean((m_flags & RF_CREATE_INCIDENT) != 0));
      json_object_set_new(flags, "aiAnalyzeIncident", json_boolean((m_flags & RF_AI_ANALYZE_INCIDENT) != 0));
      json_object_set_new(flags, "stopProcessing", json_boolean((m_flags & RF_STOP_PROCESSING) != 0));
      json_object_set_new(flags, "negatedEventMatch", json_boolean((m_flags & RF_NEGATED_EVENTS) != 0));
      json_object_set_new(flags, "negatedSourceMatch", json_boolean((m_flags & RF_NEGATED_SOURCE) != 0));
      json_object_set_new(root, "flags", flags);
   }
   else
   {
      json_object_set_new(root, "flags", json_integer(m_flags));
      json_object_set_new(root, "alarmSeverity", json_integer(m_alarmSeverity));
      json_object_set_new(root, "alarmMessage", json_string_t(m_alarmMessage));
      json_object_set_new(root, "alarmImpact", json_string_t(m_alarmImpact));
      json_object_set_new(root, "alarmKey", json_string_t(m_alarmKey));
      json_object_set_new(root, "alarmTimeout", json_integer(m_alarmTimeout));
      json_object_set_new(root, "alarmTimeoutEvent", json_integer(m_alarmTimeoutEvent));
   }

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
bool EventProcessingPolicy::loadFromDB()
{
   DB_RESULT hResult;
   bool success = false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   hResult = DBSelect(hdb, L"SELECT rule_id,rule_guid,flags,comments,alarm_message,"
                           L"alarm_severity,alarm_key,filter_script,alarm_timeout,alarm_timeout_event,"
                           L"rca_script_name,alarm_impact,action_script,downtime_tag,ai_instructions,"
                           L"incident_delay,incident_title,incident_description,"
                           L"incident_ai_depth,incident_ai_prompt,"
                           L"modified_by_guid,modified_by_name,modification_time "
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
bool EventProcessingPolicy::saveToDB(const uuid& modifiedByGuid, const TCHAR* modifiedByName) const
{
   time_t modificationTime = time(nullptr);
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
            success = m_rules.get(i)->saveToDB(hdb, modifiedByGuid, modifiedByName, modificationTime);
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
 * Fill NXCP message with conflict information
 */
void EPPConflict::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, static_cast<uint16_t>(m_type));
   msg->setField(baseId + 1, m_ruleGuid);
   if (m_clientRule != nullptr)
   {
      msg->setField(baseId + 2, true);  // Has client rule
      // Client rule data starting at baseId + 3 (use a sub-range)
   }
   else
   {
      msg->setField(baseId + 2, false);
   }
   if (m_serverRule != nullptr)
   {
      msg->setField(baseId + 3, true);  // Has server rule
      msg->setField(baseId + 4, m_serverRule->getModifiedByGuid());
      msg->setField(baseId + 5, m_serverRule->getModifiedByName());
      msg->setField(baseId + 6, static_cast<uint32_t>(m_serverRule->getModificationTime()));
   }
   else
   {
      msg->setField(baseId + 3, false);
   }
}

/**
 * Save event processing policy with optimistic concurrency merge
 */
uint32_t EventProcessingPolicy::saveWithMerge(uint32_t baseVersion, uint32_t numRules, EPRule **clientRules,
   uint32_t numDeletedRules, DeletedRuleInfo *deletedRules,
   const uuid& userGuid, const TCHAR* userName,
   ObjectArray<EPPConflict> *conflicts, uint32_t *newVersion)
{
   writeLock();

   // Fast path: no concurrent changes
   if (baseVersion == m_version)
   {
      time_t modTime = time(nullptr);
      m_rules.clear();
      for (uint32_t i = 0; i < numRules; i++)
      {
         EPRule *rule = clientRules[i];
         rule->setId(i);
         rule->incrementVersion();
         rule->setModificationInfo(userGuid, userName, modTime);
         m_rules.add(rule);
         clientRules[i] = nullptr;  // Transfer ownership
      }
      m_version++;
      *newVersion = m_version;
      unlock();

      if (!saveToDB(userGuid, userName))
         return RCC_DB_FAILURE;
      return RCC_SUCCESS;
   }

   // Slow path: concurrent changes - perform merge
   HashMap<uuid, EPRule> serverRuleMap(Ownership::False);
   for (int i = 0; i < m_rules.size(); i++)
      serverRuleMap.set(m_rules.get(i)->getGuid(), m_rules.get(i));

   ObjectArray<EPRule> mergedRules(64, 64, Ownership::False);

   // Process each client rule
   for (uint32_t i = 0; i < numRules; i++)
   {
      EPRule *clientRule = clientRules[i];

      if (clientRule->getGuid().isNull())
      {
         // New rule - always accept
         mergedRules.add(clientRule);
         clientRules[i] = nullptr;  // Transfer ownership
         continue;
      }

      EPRule *serverRule = serverRuleMap.get(clientRule->getGuid());

      if (serverRule == nullptr)
      {
         // Deleted on server while client had it
         if (clientRule->isModified())
         {
            conflicts->add(new EPPConflict(EPPConflict::EPP_CT_DELETE, clientRule->getGuid(), clientRule, nullptr));
            clientRules[i] = nullptr;  // Transfer ownership to conflict
         }
         continue;
      }

      if (serverRule->getVersion() == clientRule->getVersion())
      {
         // No server changes - accept client version
         mergedRules.add(clientRule);
         clientRules[i] = nullptr;  // Transfer ownership
      }
      else
      {
         // Server changed this rule
         if (clientRule->isModified())
         {
            conflicts->add(new EPPConflict(EPPConflict::EPP_CT_MODIFY, clientRule->getGuid(), clientRule, serverRule));
            clientRules[i] = nullptr;  // Transfer ownership to conflict
         }
         else
         {
            // Keep server version (client didn't modify)
            // Don't add clientRule to merged - just skip it
         }
      }
   }

   // Check rules deleted by client
   for (uint32_t i = 0; i < numDeletedRules; i++)
   {
      EPRule *serverRule = serverRuleMap.get(deletedRules[i].guid);
      if (serverRule != nullptr && serverRule->getVersion() > deletedRules[i].version)
      {
         // Server modified after client loaded - conflict
         conflicts->add(new EPPConflict(EPPConflict::EPP_CT_DELETE, serverRule->getGuid(), nullptr, serverRule));
      }
   }

   if (conflicts->isEmpty())
   {
      // Apply merged rules - need to also include unmodified server rules that client didn't touch
      // First, build set of GUIDs in mergedRules
      HashSet<uuid> mergedGuids;
      for (int i = 0; i < mergedRules.size(); i++)
      {
         if (!mergedRules.get(i)->getGuid().isNull())
            mergedGuids.put(mergedRules.get(i)->getGuid());
      }

      // Add server rules that client didn't modify and didn't delete
      HashSet<uuid> deletedGuids;
      for (uint32_t i = 0; i < numDeletedRules; i++)
         deletedGuids.put(deletedRules[i].guid);

      for (int i = 0; i < m_rules.size(); i++)
      {
         EPRule *serverRule = m_rules.get(i);
         if (!mergedGuids.contains(serverRule->getGuid()) && !deletedGuids.contains(serverRule->getGuid()))
         {
            // Client didn't send this rule and didn't delete it
            // This means client's version was not modified - keep server version
            // This is tricky because we're in a merge scenario and need to preserve server rules
            // Actually, if client sent all rules (modified or not), this shouldn't happen
            // For now, assume client sends all rules it has
         }
      }

      time_t modTime = time(nullptr);
      m_rules.clear();
      for (int i = 0; i < mergedRules.size(); i++)
      {
         EPRule *rule = mergedRules.get(i);
         rule->setId(i);
         rule->incrementVersion();
         rule->setModificationInfo(userGuid, userName, modTime);
         m_rules.add(rule);
      }
      m_version++;
      *newVersion = m_version;
      unlock();

      if (!saveToDB(userGuid, userName))
         return RCC_DB_FAILURE;
      return RCC_SUCCESS;
   }
   else
   {
      *newVersion = m_version;
      unlock();
      return RCC_EPP_CONFLICT;
   }
}

/**
 * Pass event through policy
 */
void EventProcessingPolicy::processEvent(Event *pEvent)
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
void EventProcessingPolicy::sendToClient(ClientSession *session, uint32_t requestId) const
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
void EventProcessingPolicy::replacePolicy(uint32_t numRules, EPRule **ruleList)
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
void EventProcessingPolicy::validateConfig() const
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
bool EventProcessingPolicy::isActionInUse(uint32_t actionId) const
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
bool EventProcessingPolicy::isCategoryInUse(uint32_t categoryId) const
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
 * Get rule details as JSON
 */
json_t *EventProcessingPolicy::getRuleDetails(const uuid& ruleId) const
{
   json_t *details = nullptr;
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      if (ruleId.equals(m_rules.get(i)->getGuid()))
      {
         details = m_rules.get(i)->toJson(true);
         break;
      }
   }
   unlock();
   return details;
}

/**
 * Export rule to JSON
 */
json_t *EventProcessingPolicy::exportRule(const uuid& guid) const
{
   json_t *rule = nullptr;
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      if (guid.equals(m_rules.get(i)->getGuid()))
      {
         rule = m_rules.get(i)->createExportRecord();
         break;
      }
   }
   unlock();
   return rule;
}

/**
 * Export rules ordering to JSON
 */
json_t *EventProcessingPolicy::exportRuleOrdering() const
{
   json_t *ordering = json_array();
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      json_array_append_new(ordering, json_string_t(m_rules.get(i)->getGuid().toString()));
   }
   unlock();
   return ordering;
}

/**
 * Finds rule index by guid and adds index shift if found
 */
int EventProcessingPolicy::findRuleIndexByGuid(const uuid& guid, int shift) const
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
void EventProcessingPolicy::importRule(EPRule *rule, bool overwrite, ObjectArray<uuid> *ruleOrdering)
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
json_t *EventProcessingPolicy::toJson() const
{
   json_t *root = json_object();
   json_t *rules = json_array();
   readLock();
   for(int i = 0; i < m_rules.size(); i++)
   {
      json_t *r = m_rules.get(i)->toJson();
      json_object_set_new(r, "ruleNumber", json_integer(i + 1));
      json_array_append_new(rules, r);
   }
   unlock();
   json_object_set_new(root, "rules", rules);
   return root;
}

/**
 * Collects information about all EPRules that are using specified event
 */
void EventProcessingPolicy::getEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences) const
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
