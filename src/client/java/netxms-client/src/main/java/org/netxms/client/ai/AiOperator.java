/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.ai;

import java.util.Date;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * AI operator instance - perpetual adaptive monitoring loop. Configuration fields have setters
 * and can be changed by the client; adaptive state fields (current focus, watch list, memento)
 * are managed by the operator itself and are read-only.
 */
public class AiOperator
{
   private int id;
   private String name;
   private String description;
   private int ownerUserId;
   private boolean enabled;
   private boolean executing;
   private String scopeFilter;
   private String modelSlot;
   private int minInterval;
   private int maxInterval;
   private int dailyTokenBudget;
   private long tokensUsedToday;
   private String personaPrompt;
   private String currentFocus;
   private String watchList;
   private String memento;
   private int observationRetentionDays;
   private int observationMaxRecords;
   private Date lastExecutionTime;
   private Date nextExecutionTime;
   private int iteration;
   private int consecutiveFailures;
   private String lastExplanation;
   private Date creationTime;
   private Date modificationTime;

   /**
    * Create new AI operator instance definition with default settings (for subsequent creation on server).
    *
    * @param name instance name
    */
   public AiOperator(String name)
   {
      id = 0;
      this.name = name;
      description = "";
      ownerUserId = 0;
      enabled = true;
      executing = false;
      scopeFilter = "";
      modelSlot = "";
      minInterval = 300;
      maxInterval = 3600;
      dailyTokenBudget = 0;
      tokensUsedToday = 0;
      personaPrompt = "";
      currentFocus = "";
      watchList = "";
      memento = "";
      observationRetentionDays = 0;
      observationMaxRecords = 0;
      lastExecutionTime = null;
      nextExecutionTime = null;
      iteration = 0;
      consecutiveFailures = 0;
      lastExplanation = "";
      creationTime = null;
      modificationTime = null;
   }

   /**
    * Create AI operator instance object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base ID for fields
    */
   public AiOperator(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      name = msg.getFieldAsString(baseId + 1);
      description = msg.getFieldAsString(baseId + 2);
      ownerUserId = msg.getFieldAsInt32(baseId + 3);
      enabled = msg.getFieldAsBoolean(baseId + 4);
      executing = msg.getFieldAsBoolean(baseId + 5);
      scopeFilter = msg.getFieldAsString(baseId + 6);
      modelSlot = msg.getFieldAsString(baseId + 7);
      minInterval = msg.getFieldAsInt32(baseId + 8);
      maxInterval = msg.getFieldAsInt32(baseId + 9);
      dailyTokenBudget = msg.getFieldAsInt32(baseId + 10);
      tokensUsedToday = msg.getFieldAsInt64(baseId + 11);
      personaPrompt = msg.getFieldAsString(baseId + 12);
      currentFocus = msg.getFieldAsString(baseId + 13);
      watchList = msg.getFieldAsString(baseId + 14);
      memento = msg.getFieldAsString(baseId + 15);
      observationRetentionDays = msg.getFieldAsInt32(baseId + 16);
      observationMaxRecords = msg.getFieldAsInt32(baseId + 17);
      lastExecutionTime = msg.getFieldAsDate(baseId + 18);
      nextExecutionTime = msg.getFieldAsDate(baseId + 19);
      iteration = msg.getFieldAsInt32(baseId + 20);
      consecutiveFailures = msg.getFieldAsInt32(baseId + 21);
      lastExplanation = msg.getFieldAsString(baseId + 22);
      creationTime = msg.getFieldAsDate(baseId + 23);
      modificationTime = msg.getFieldAsDate(baseId + 24);
   }

   /**
    * Fill NXCP message with configuration data for create/modify request.
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldUInt32(NXCPCodes.VID_AI_OPERATOR_ID, id);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_DESCRIPTION, description);
      msg.setField(NXCPCodes.VID_ENABLED, enabled);
      msg.setField(NXCPCodes.VID_FILTER, scopeFilter);
      msg.setField(NXCPCodes.VID_AI_MODEL_SLOT, modelSlot);
      msg.setFieldInt32(NXCPCodes.VID_MIN_INTERVAL, minInterval);
      msg.setFieldInt32(NXCPCodes.VID_MAX_INTERVAL, maxInterval);
      msg.setFieldInt32(NXCPCodes.VID_AI_TOKEN_BUDGET, dailyTokenBudget);
      msg.setField(NXCPCodes.VID_PROMPT, personaPrompt);
      msg.setFieldInt32(NXCPCodes.VID_RETENTION_TIME, observationRetentionDays);
      msg.setFieldInt32(NXCPCodes.VID_MAX_RECORDS, observationMaxRecords);
   }

   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * @return the ownerUserId
    */
   public int getOwnerUserId()
   {
      return ownerUserId;
   }

   /**
    * @return true if the instance is enabled
    */
   public boolean isEnabled()
   {
      return enabled;
   }

   /**
    * @param enabled true to enable the instance
    */
   public void setEnabled(boolean enabled)
   {
      this.enabled = enabled;
   }

   /**
    * @return true if an iteration is currently running
    */
   public boolean isExecuting()
   {
      return executing;
   }

   /**
    * @return the scopeFilter
    */
   public String getScopeFilter()
   {
      return scopeFilter;
   }

   /**
    * @param scopeFilter the scopeFilter to set
    */
   public void setScopeFilter(String scopeFilter)
   {
      this.scopeFilter = scopeFilter;
   }

   /**
    * @return the modelSlot
    */
   public String getModelSlot()
   {
      return modelSlot;
   }

   /**
    * @param modelSlot the modelSlot to set
    */
   public void setModelSlot(String modelSlot)
   {
      this.modelSlot = modelSlot;
   }

   /**
    * @return the minInterval
    */
   public int getMinInterval()
   {
      return minInterval;
   }

   /**
    * @param minInterval the minInterval to set
    */
   public void setMinInterval(int minInterval)
   {
      this.minInterval = minInterval;
   }

   /**
    * @return the maxInterval
    */
   public int getMaxInterval()
   {
      return maxInterval;
   }

   /**
    * @param maxInterval the maxInterval to set
    */
   public void setMaxInterval(int maxInterval)
   {
      this.maxInterval = maxInterval;
   }

   /**
    * @return the dailyTokenBudget
    */
   public int getDailyTokenBudget()
   {
      return dailyTokenBudget;
   }

   /**
    * @param dailyTokenBudget the dailyTokenBudget to set
    */
   public void setDailyTokenBudget(int dailyTokenBudget)
   {
      this.dailyTokenBudget = dailyTokenBudget;
   }

   /**
    * @return LLM tokens used within the current usage day (UTC)
    */
   public long getTokensUsedToday()
   {
      return tokensUsedToday;
   }

   /**
    * @return the personaPrompt
    */
   public String getPersonaPrompt()
   {
      return personaPrompt;
   }

   /**
    * @param personaPrompt the personaPrompt to set
    */
   public void setPersonaPrompt(String personaPrompt)
   {
      this.personaPrompt = personaPrompt;
   }

   /**
    * @return current focus set by the operator itself
    */
   public String getCurrentFocus()
   {
      return currentFocus;
   }

   /**
    * @return watch list maintained by the operator itself
    */
   public String getWatchList()
   {
      return watchList;
   }

   /**
    * @return state carried between iterations by the operator itself
    */
   public String getMemento()
   {
      return memento;
   }

   /**
    * @return the observationRetentionDays
    */
   public int getObservationRetentionDays()
   {
      return observationRetentionDays;
   }

   /**
    * @param observationRetentionDays the observationRetentionDays to set
    */
   public void setObservationRetentionDays(int observationRetentionDays)
   {
      this.observationRetentionDays = observationRetentionDays;
   }

   /**
    * @return the observationMaxRecords
    */
   public int getObservationMaxRecords()
   {
      return observationMaxRecords;
   }

   /**
    * @param observationMaxRecords the observationMaxRecords to set
    */
   public void setObservationMaxRecords(int observationMaxRecords)
   {
      this.observationMaxRecords = observationMaxRecords;
   }

   /**
    * @return the lastExecutionTime
    */
   public Date getLastExecutionTime()
   {
      return lastExecutionTime;
   }

   /**
    * @return the nextExecutionTime
    */
   public Date getNextExecutionTime()
   {
      return nextExecutionTime;
   }

   /**
    * @return number of completed iterations
    */
   public int getIteration()
   {
      return iteration;
   }

   /**
    * @return number of consecutive failed executions
    */
   public int getConsecutiveFailures()
   {
      return consecutiveFailures;
   }

   /**
    * @return explanation returned by the last execution
    */
   public String getLastExplanation()
   {
      return lastExplanation;
   }

   /**
    * @return the creationTime
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return the modificationTime
    */
   public Date getModificationTime()
   {
      return modificationTime;
   }
}
