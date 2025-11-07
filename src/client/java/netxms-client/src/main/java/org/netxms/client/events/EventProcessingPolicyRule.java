/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.events;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.Severity;

/**
 * This class represents single rule of event processing policy.
 */
public class EventProcessingPolicyRule
{
   // Rule flags (options)
   public static final int STOP_PROCESSING     = 0x000001;
   public static final int NEGATED_SOURCE      = 0x000002;
   public static final int NEGATED_EVENTS      = 0x000004;
   public static final int GENERATE_ALARM      = 0x000008;
   public static final int DISABLED            = 0x000010;
   public static final int TERMINATE_BY_REGEXP = 0x000020;
   public static final int SEVERITY_NORMAL     = 0x000100;
   public static final int SEVERITY_WARNING    = 0x000200;
   public static final int SEVERITY_MINOR      = 0x000400;
   public static final int SEVERITY_MAJOR      = 0x000800;
   public static final int SEVERITY_CRITICAL   = 0x001000;
   public static final int CREATE_TICKET       = 0x002000;
   public static final int ACCEPT_CORRELATED   = 0x004000;
   public static final int NEGATED_TIME_FRAMES = 0x008000;
   public static final int START_DOWNTIME      = 0x010000;
   public static final int END_DOWNTIME        = 0x020000;
   public static final int REQUEST_AI_COMMENT  = 0x040000;

   public static final int SEVERITY_ANY = SEVERITY_NORMAL | SEVERITY_WARNING | SEVERITY_MINOR | SEVERITY_MAJOR | SEVERITY_CRITICAL;

   private UUID guid;
   private List<Long> sources;
   private List<Long> sourceExclusions;
   private List<Integer> events;
   private String filterScript;
   private List<TimeFrame> timeFrames;
   private int flags;
   private String alarmKey;
   private String alarmMessage;
   private Severity alarmSeverity;
   private int alarmTimeout;
   private int alarmTimeoutEvent;
   private List<Long> alarmCategoryIds;
   private String rcaScriptName;
   private String downtimeTag;
   private String actionScript;
   private List<ActionExecutionConfiguration> actions;
   private List<String> timerCancellations;
   private Map<String, String> persistentStorageSet;
   private List<String> persistentStorageDelete;
   private Map<String, String> customAttributeStorageSet;
   private List<String> customAttributeStorageDelete;
   private String aiAgentInstructions;
   private String comments;
   private int ruleNumber;

   /**
    * Create empty rule
    */
   public EventProcessingPolicyRule()
   {
      guid = UUID.randomUUID();
      sources = new ArrayList<Long>(0);
      sourceExclusions = new ArrayList<Long>(0);
      events = new ArrayList<Integer>(0);
      filterScript = "";
      timeFrames = new ArrayList<TimeFrame>();
      flags = SEVERITY_ANY;
      alarmKey = "";
      alarmMessage = "%m";
      alarmSeverity = Severity.UNKNOWN;
      alarmTimeout = 0;
      alarmTimeoutEvent = 43;
      alarmCategoryIds = new ArrayList<Long>(0);
      rcaScriptName = null;
      downtimeTag = "";
      actionScript = null;
      actions = new ArrayList<ActionExecutionConfiguration>(0);
      timerCancellations = new ArrayList<String>(0);
      persistentStorageSet = new HashMap<String, String>(0);
      persistentStorageDelete = new ArrayList<String>(0);
      customAttributeStorageSet = new HashMap<String, String>(0);
      customAttributeStorageDelete = new ArrayList<String>(0);
      aiAgentInstructions = "";
      comments = "";
      ruleNumber = 0;
   }

   /**
    * Copy constructor
    * 
    * @param src source rule
    */
   public EventProcessingPolicyRule(EventProcessingPolicyRule src)
   {
      guid = UUID.randomUUID();
      sources = new ArrayList<Long>(src.sources);
      sourceExclusions = new ArrayList<Long>(src.sourceExclusions);
      events = new ArrayList<Integer>(src.events);
      filterScript = src.filterScript;
      timeFrames = new ArrayList<TimeFrame>(src.timeFrames.size());
      for(TimeFrame d : src.timeFrames)
         timeFrames.add(new TimeFrame(d));
      flags = src.flags;
      alarmKey = src.alarmKey;
      alarmMessage = src.alarmMessage;
      alarmSeverity = src.alarmSeverity;
      alarmTimeout = src.alarmTimeout;
      alarmTimeoutEvent = src.alarmTimeoutEvent;
      alarmCategoryIds = src.alarmCategoryIds;
      rcaScriptName = src.rcaScriptName;
      downtimeTag = src.downtimeTag;
      actionScript = src.actionScript;
      actions = new ArrayList<ActionExecutionConfiguration>(src.actions.size());
      for(ActionExecutionConfiguration d : src.actions)
         actions.add(new ActionExecutionConfiguration(d));
      timerCancellations = new ArrayList<String>(src.timerCancellations);
      persistentStorageSet = new HashMap<String, String>(src.persistentStorageSet);
      persistentStorageDelete = new ArrayList<String>(src.persistentStorageDelete);
      customAttributeStorageSet = new HashMap<String, String>(src.customAttributeStorageSet);
      customAttributeStorageDelete = new ArrayList<String>(src.customAttributeStorageDelete);
      aiAgentInstructions = src.aiAgentInstructions;
      comments = src.comments;
      ruleNumber = src.ruleNumber;
   }

   /**
    * Create rule from NXCP message.
    *
    * @param msg NXCP message
    * @param ruleNumber this rule number
    */
   public EventProcessingPolicyRule(NXCPMessage msg, int ruleNumber)
   {
      guid = msg.getFieldAsUUID(NXCPCodes.VID_GUID);
      sources = Arrays.asList(msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_RULE_SOURCES));
      sourceExclusions = Arrays.asList(msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_RULE_SOURCE_EXCLUSIONS));
      events = Arrays.asList(msg.getFieldAsInt32ArrayEx(NXCPCodes.VID_RULE_EVENTS));
      filterScript = msg.getFieldAsString(NXCPCodes.VID_SCRIPT);
      int frameCount = msg.getFieldAsInt32(NXCPCodes.VID_NUM_TIME_FRAMES);
      timeFrames = new ArrayList<TimeFrame>(frameCount);
      long fieldId = NXCPCodes.VID_TIME_FRAME_LIST_BASE;
      for(int i = 0; i < frameCount; i++)
      {
         timeFrames.add(new TimeFrame(msg.getFieldAsInt32(fieldId++), msg.getFieldAsInt64(fieldId++)));
      }  
      flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
      alarmKey = msg.getFieldAsString(NXCPCodes.VID_ALARM_KEY);
      alarmMessage = msg.getFieldAsString(NXCPCodes.VID_ALARM_MESSAGE);
      alarmSeverity = Severity.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_ALARM_SEVERITY));
      alarmTimeout = msg.getFieldAsInt32(NXCPCodes.VID_ALARM_TIMEOUT);
      alarmTimeoutEvent = msg.getFieldAsInt32(NXCPCodes.VID_ALARM_TIMEOUT_EVENT);
      alarmCategoryIds = Arrays.asList(msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_ALARM_CATEGORY_ID));
      rcaScriptName = msg.getFieldAsString(NXCPCodes.VID_RCA_SCRIPT_NAME);
      downtimeTag = msg.getFieldAsString(NXCPCodes.VID_DOWNTIME_TAG);
      actionScript = msg.getFieldAsString(NXCPCodes.VID_ACTION_SCRIPT);
      aiAgentInstructions = msg.getFieldAsString(NXCPCodes.VID_AI_AGENT_INSTRUCTIONS);
      comments = msg.getFieldAsString(NXCPCodes.VID_COMMENTS);

      int actionCount = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ACTIONS);
      actions = new ArrayList<ActionExecutionConfiguration>(actionCount);
      fieldId = NXCPCodes.VID_ACTION_LIST_BASE;
      for(int i = 0; i < actionCount; i++)
      {
         actions.add(new ActionExecutionConfiguration(msg, fieldId));
         fieldId += 10;
      }
      timerCancellations = msg.getStringListFromField(NXCPCodes.VID_TIMER_LIST);

      persistentStorageSet = msg.getStringMapFromFields(NXCPCodes.VID_PSTORAGE_SET_LIST_BASE, NXCPCodes.VID_NUM_SET_PSTORAGE);
      persistentStorageDelete = msg.getStringListFromFields(NXCPCodes.VID_PSTORAGE_DELETE_LIST_BASE, NXCPCodes.VID_NUM_DELETE_PSTORAGE);

      customAttributeStorageSet = msg.getStringMapFromFields(NXCPCodes.VID_CUSTOM_ATTR_SET_LIST_BASE, NXCPCodes.VID_CUSTOM_ATTR_SET_COUNT);
      customAttributeStorageDelete = msg.getStringListFromFields(NXCPCodes.VID_CUSTOM_ATTR_DEL_LIST_BASE, NXCPCodes.VID_CUSTOM_ATTR_DEL_COUNT);

      this.ruleNumber = ruleNumber;
   }

   /**
    * Fill NXCP message with rule's data
    *
    * @param msg NXCP message
    */
   public void fillMessage(final NXCPMessage msg)
   {
      msg.setField(NXCPCodes.VID_GUID, guid);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
      msg.setField(NXCPCodes.VID_COMMENTS, comments);
      msg.setField(NXCPCodes.VID_SCRIPT, filterScript);

      msg.setFieldInt32(NXCPCodes.VID_NUM_ACTIONS, actions.size());
      long fieldId = NXCPCodes.VID_ACTION_LIST_BASE;
      for(ActionExecutionConfiguration d : actions)
      {
         d.fillMessage(msg, fieldId);
         fieldId += 10;
      }
      msg.setFieldStringCollection(NXCPCodes.VID_TIMER_LIST, timerCancellations);

      msg.setField(NXCPCodes.VID_RULE_EVENTS, events);
      msg.setField(NXCPCodes.VID_RULE_SOURCES, sources);
      msg.setField(NXCPCodes.VID_RULE_SOURCE_EXCLUSIONS, sourceExclusions);
      msg.setFieldInt32(NXCPCodes.VID_NUM_TIME_FRAMES, timeFrames.size());
      fieldId = NXCPCodes.VID_TIME_FRAME_LIST_BASE;
      for(TimeFrame frame : timeFrames)
      {
         frame.fillMessage(msg, fieldId);
         fieldId += 2;
      }

      msg.setField(NXCPCodes.VID_ALARM_KEY, alarmKey);
      msg.setField(NXCPCodes.VID_ALARM_MESSAGE, alarmMessage);
      msg.setFieldInt16(NXCPCodes.VID_ALARM_SEVERITY, alarmSeverity.getValue());
      msg.setFieldInt32(NXCPCodes.VID_ALARM_TIMEOUT, alarmTimeout);
      msg.setFieldInt32(NXCPCodes.VID_ALARM_TIMEOUT_EVENT, alarmTimeoutEvent);
      msg.setField(NXCPCodes.VID_ALARM_CATEGORY_ID, alarmCategoryIds);
      msg.setField(NXCPCodes.VID_RCA_SCRIPT_NAME, rcaScriptName);
      msg.setField(NXCPCodes.VID_DOWNTIME_TAG, downtimeTag);
      msg.setField(NXCPCodes.VID_ACTION_SCRIPT, actionScript);
      msg.setField(NXCPCodes.VID_AI_AGENT_INSTRUCTIONS, aiAgentInstructions);

      msg.setFieldsFromStringMap(persistentStorageSet, NXCPCodes.VID_PSTORAGE_SET_LIST_BASE, NXCPCodes.VID_NUM_SET_PSTORAGE);
      msg.setFieldsFromStringCollection(persistentStorageDelete, NXCPCodes.VID_PSTORAGE_DELETE_LIST_BASE, NXCPCodes.VID_NUM_DELETE_PSTORAGE);

      msg.setFieldsFromStringMap(customAttributeStorageSet, NXCPCodes.VID_CUSTOM_ATTR_SET_LIST_BASE, NXCPCodes.VID_CUSTOM_ATTR_SET_COUNT);
      msg.setFieldsFromStringCollection(customAttributeStorageDelete, NXCPCodes.VID_CUSTOM_ATTR_DEL_LIST_BASE, NXCPCodes.VID_CUSTOM_ATTR_DEL_COUNT);
   }

   /**
    * Get rule's comments.
    *
    * @return Rule's comments
    */
   public String getComments()
   {
      return comments;
   }

   /**
    * Set rule's comments.
    *
    * @param comments New comments
    */
   public void setComments(String comments)
   {
      this.comments = comments;
   }

   /**
    * @return the script
    */
   public String getFilterScript()
   {
      return filterScript;
   }

   /**
    * @param script the script to set
    */
   public void setFilterScript(String script)
   {
      this.filterScript = script;
   }

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * @param flags the flags to set
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }

   /**
    * @return the alarmKey
    */
   public String getAlarmKey()
   {
      return alarmKey;
   }

   /**
    * @param alarmKey the alarmKey to set
    */
   public void setAlarmKey(String alarmKey)
   {
      this.alarmKey = alarmKey;
   }

   /**
    * @return the alarmMessage
    */
   public String getAlarmMessage()
   {
      return alarmMessage;
   }

   /**
    * @param alarmMessage the alarmMessage to set
    */
   public void setAlarmMessage(String alarmMessage)
   {
      this.alarmMessage = alarmMessage;
   }

   /**
    * @return the alarmSeverity
    */
   public Severity getAlarmSeverity()
   {
      return alarmSeverity;
   }

   /**
    * @param alarmSeverity the alarmSeverity to set
    */
   public void setAlarmSeverity(Severity alarmSeverity)
   {
      this.alarmSeverity = alarmSeverity;
   }

   /**
    * @return the alarmTimeout
    */
   public int getAlarmTimeout()
   {
      return alarmTimeout;
   }

   /**
    * @param alarmTimeout the alarmTimeout to set
    */
   public void setAlarmTimeout(int alarmTimeout)
   {
      this.alarmTimeout = alarmTimeout;
   }

   /**
    * @return the alarmTimeoutEvent
    */
   public int getAlarmTimeoutEvent()
   {
      return alarmTimeoutEvent;
   }

   /**
    * @param alarmTimeoutEvent the alarmTimeoutEvent to set
    */
   public void setAlarmTimeoutEvent(int alarmTimeoutEvent)
   {
      this.alarmTimeoutEvent = alarmTimeoutEvent;
   }

   /**
    * @return alarmCategoryIds the alarm category ids
    */
   public List<Long> getAlarmCategories()
   {
      return alarmCategoryIds;
   }

   /**
    * @param alarmCategoryIds the alarm category ids to set
    */
   public void setAlarmCategories(List<Long> alarmCategoryIds)
   {
      this.alarmCategoryIds = alarmCategoryIds;
   }

   /**
    * Remove specific alarm category from categories list
    * 
    * @param categoryId alarm category ID
    */
   public void removeAlarmCategory(Long categoryId)
   {
      for(int i = 0; i < alarmCategoryIds.size(); i++)
      {
         if (alarmCategoryIds.get(i) == categoryId)
         {
            alarmCategoryIds.remove(i);
            break;
         }
      }

   }

   /**
    * Get name of root cause analysis script.
    * 
    * @return name of root cause analysis script (can be null or empty string if not set)
    */
   public String getRcaScriptName()
   {
      return rcaScriptName;
   }

   /**
    * Set name of root cause analysis script.
    *
    * @param rcaScriptName name of root cause analysis script (can be null or empty string to disable RCA)
    */
   public void setRcaScriptName(String rcaScriptName)
   {
      this.rcaScriptName = rcaScriptName;
   }

   /**
    * @return the sources
    */
   public List<Long> getSources()
   {
      return sources;
   }

   /**
    * @return the sourceExclusions
    */
   public List<Long> getSourceExclusions()
   {
      return sourceExclusions;
   }

   /**
    * @param sourceExclusions the sourceExclusions to set
    */
   public void setSourceExclusions(List<Long> sourceExclusions)
   {
      this.sourceExclusions = sourceExclusions;
   }

   /**
    * @return the events
    */
   public List<Integer> getEvents()
   {
      return events;
   }

   /**
    * @return the actions
    */
   public List<ActionExecutionConfiguration> getActions()
   {
      return actions;
   }

   /**
    * @return the persistentStorageSet
    */
   public Map<String, String> getPStorageSet()
   {
      return persistentStorageSet;
   }

   /**
    * @return the persistentStorageDelete
    */
   public List<String> getPStorageDelete()
   {
      return persistentStorageDelete;
   }

   /**
    * @param sources the sources to set
    */
   public void setSources(List<Long> sources)
   {
      this.sources = sources;
   }

   /**
    * @param events the events to set
    */
   public void setEvents(List<Integer> events)
   {
      this.events = events;
   }

   /**
    * @param actions the actions to set
    */
   public void setActions(List<ActionExecutionConfiguration> actions)
   {
      this.actions = actions;
   }

   /**
    * @return the timerCancellations
    */
   public List<String> getTimerCancellations()
   {
      return timerCancellations;
   }

   /**
    * @param timerCancellations the timerCancellations to set
    */
   public void setTimerCancellations(List<String> timerCancellations)
   {
      this.timerCancellations = timerCancellations;
   }

   /**
    * @param persistentStorageSet the persistentStorageSet to set
    */
   public void setPStorageSet(Map<String, String> persistentStorageSet)
   {
      this.persistentStorageSet = persistentStorageSet;
   }

   /**
    * @param persistentStorageDelete the persistentStorageDelete to set
    */
   public void setPStorageDelete(List<String> persistentStorageDelete)
   {
      this.persistentStorageDelete = persistentStorageDelete;
   }

   /**
    * Check if rule's filter is empty.
    *
    * @return true if rule's filter is empty, false otherwise
    */
   public boolean isFilterEmpty()
   {
      return sources.isEmpty() && sourceExclusions.isEmpty() && events.isEmpty() && filterScript.isBlank() && timeFrames.isEmpty() &&
         ((flags & (SEVERITY_NORMAL | SEVERITY_WARNING | SEVERITY_MINOR | SEVERITY_MAJOR | SEVERITY_CRITICAL)) == (SEVERITY_NORMAL | SEVERITY_WARNING | SEVERITY_MINOR | SEVERITY_MAJOR | SEVERITY_CRITICAL));
   }

   /**
    * Check rule's DISABLED flag
    *
    * @return true if DISABLED flag set
    */
   public boolean isDisabled()
   {
      return (flags & DISABLED) != 0;
   }

   /**
    * Check rule's NEGATED_SOURCE flag
    *
    * @return true if NEGATED_SOURCE flag set
    */
   public boolean isSourceInverted()
   {
      return (flags & NEGATED_SOURCE) != 0;
   }

   /**
    * Check rule's NEGATED_EVENTS flag
    *
    * @return true if NEGATED_EVENTS flag set
    */
   public boolean isEventsInverted()
   {
      return (flags & NEGATED_EVENTS) != 0;
   }

   /**
    * Check rule's NEGATED_TIME_FRAMES flag
    *
    * @return true if NEGATED_TIME_FRAMES flag set
    */
   public boolean isTimeFramesInverted()
   {
      return (flags & NEGATED_TIME_FRAMES) != 0;
   }

   /**
    * Check rule's ACCEPT_CORRELATED flag
    *
    * @return true if ACCEPT_CORRELATED flag set
    */
   public boolean isCorrelatedEventProcessingAllowed()
   {
      return (flags & ACCEPT_CORRELATED) != 0;
   }

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * Get rule number
    *
    * @return rule number
    */
   public int getRuleNumber()
   {
      return ruleNumber;
   }

   /**
    * Set rule number
    *
    * @param ruleNumber to set
    */
   public void setRuleNumber(int ruleNumber)
   {
      this.ruleNumber = ruleNumber;
   }

   /**
    * @return the actionScript
    */
   public String getActionScript()
   {
      return actionScript;
   }

   /**
    * @param actionScript the actionScript to set
    */
   public void setActionScript(String actionScript)
   {
      this.actionScript = actionScript;
   }

   /**
    * @return the customAttributeStorageSet
    */
   public Map<String, String> getCustomAttributeStorageSet()
   {
      return customAttributeStorageSet;
   }

   /**
    * @param customAttributeStorageSet the customAttributeStorageSet to set
    */
   public void setCustomAttributeStorageSet(Map<String, String> customAttributeStorageSet)
   {
      this.customAttributeStorageSet = customAttributeStorageSet;
   }

   /**
    * @return the customAttributeStorageDelete
    */
   public List<String> getCustomAttributeStorageDelete()
   {
      return customAttributeStorageDelete;
   }

   /**
    * @param customAttributeStorageDelete the customAttributeStorageDelete to set
    */
   public void setCustomAttributeStorageDelete(List<String> customAttributeStorageDelete)
   {
      this.customAttributeStorageDelete = customAttributeStorageDelete;
   }

   /**
    * @return the timeFrames
    */
   public List<TimeFrame> getTimeFrames()
   {
      return timeFrames;
   }

   /**
    * @param timeFrames the timeFrames to set
    */
   public void setTimeFrames(List<TimeFrame> timeFrames)
   {
      this.timeFrames = timeFrames;
   }

   /**
    * @return the downtimeTag
    */
   public String getDowntimeTag()
   {
      return downtimeTag;
   }

   /**
    * @param downtimeTag the downtimeTag to set
    */
   public void setDowntimeTag(String downtimeTag)
   {
      this.downtimeTag = downtimeTag;
   }

   /**
    * Get instructions for AI agent
    *
    * @return instructions for AI agent
    */
   public String getAiAgentInstructions()
   {
      return aiAgentInstructions;
   }

   /**
    * Set instructions for AI agent
    *
    * @param aiAgentInstructions AI agent instructions to set
    */
   public void setAiAgentInstructions(String aiAgentInstructions)
   {
      this.aiAgentInstructions = aiAgentInstructions;
   }
}
