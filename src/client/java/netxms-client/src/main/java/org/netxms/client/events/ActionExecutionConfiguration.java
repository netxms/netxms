/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2021 Victor Kirhenshtein
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
package org.netxms.client.events;

import org.netxms.base.NXCPMessage;

/**
 * Action execution data
 */
public class ActionExecutionConfiguration
{   
   private long actionId;
   private String timerDelay;
   private String snoozeTime;
   private String timerKey;
   private String blockingTimerKey;
   private boolean active;
   
   /**
    * Create new action execution configuration without timer delay and key.
    * 
    * @param actionId action ID
    */
   ActionExecutionConfiguration(long actionId)
   {
      this.actionId = actionId;
      this.timerDelay = null;
      this.snoozeTime = null;
      this.timerKey = null;
      this.blockingTimerKey = null;
      active = true;
   }
   
   /**
    * Create new action execution configuration
    * 
    * @param actionId action ID
    * @param timerDelay timer delay in seconds
    * @param snoozeTime snoozeTime in seconds
    * @param timerKey timer key
    * @param blockingTimerKey timer key for blocking the action
    */
   public ActionExecutionConfiguration(long actionId, String timerDelay, String snoozeTime, String timerKey, String blockingTimerKey)
   {
      this.actionId = actionId;
      this.timerDelay = timerDelay;
      this.snoozeTime = snoozeTime;
      this.timerKey = timerKey;
      this.blockingTimerKey = blockingTimerKey;
      active = true;
   }

   /**
    * Create action execution configuration from NXCP message 
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected ActionExecutionConfiguration(NXCPMessage msg, long baseId)
   {
      actionId = msg.getFieldAsInt64(baseId);
      timerDelay = msg.getFieldAsString(baseId + 1);
      timerKey = msg.getFieldAsString(baseId + 2);
      blockingTimerKey = msg.getFieldAsString(baseId + 3);
      snoozeTime = msg.getFieldAsString(baseId + 4);
      active = msg.getFieldAsBoolean(baseId + 5);
   }

   /**
    * Copy constructor
    * 
    * @param src source object
    */
   public ActionExecutionConfiguration(ActionExecutionConfiguration src)
   {
      this.actionId = src.actionId;
      this.timerDelay = src.timerDelay;
      this.snoozeTime = src.snoozeTime;
      this.timerKey = src.timerKey;
      this.blockingTimerKey = src.blockingTimerKey;
      this.active = src.active;
   }

   /**
    * Fill NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setFieldInt32(baseId, (int)actionId);
      msg.setField(baseId + 1, timerDelay);
      msg.setField(baseId + 2, timerKey);
      msg.setField(baseId + 3, blockingTimerKey);
      msg.setField(baseId + 4, snoozeTime);
      msg.setField(baseId + 5, active);
   }
   
   /**
    * Get timer delay.
    * 
    * @return timer delay in seconds
    */
   public String getTimerDelay()
   {
      return (timerDelay != null) ? timerDelay : "";
   }

   /**
    * Set timer delay.
    * 
    * @param timerDelay timer delay in seconds
    */
   public void setTimerDelay(String timerDelay)
   {
      this.timerDelay = timerDelay;
   }

   /**
    * Get timer key
    * 
    * @return current timer key
    */
   public String getTimerKey()
   {
      return (timerKey != null) ? timerKey : "";
   }

   /**
    * Set timer key
    * 
    * @param timerKey new timer key
    */
   public void setTimerKey(String timerKey)
   {
      this.timerKey = timerKey;
   }

   /**
    * Get action ID
    * 
    * @return action ID
    */
   public long getActionId()
   {
      return actionId;
   }

   /**
    * Get blocking timer key.
    *
    * @return blocking timer key or empty string if not set
    */
   public String getBlockingTimerKey()
   {
      return (blockingTimerKey != null) ? blockingTimerKey : "";
   }

   /**
    * Set blocking timer key
    *
    * @param blockingTimerKey blocking timer key
    */
   public void setBlockingTimerKey(String blockingTimerKey)
   {
      this.blockingTimerKey = blockingTimerKey;
   }

   /**
    * @return the snoozeTime
    */
   public String getSnoozeTime()
   {
      return (snoozeTime != null) ? snoozeTime : "";
   }

   /**
    * @param snoozeTime the snoozeTime to set
    */
   public void setSnoozeTime(String snoozeTime)
   {
      this.snoozeTime = snoozeTime;
   }
   
   /**
    * If action should be executed
    * 
    * @return true if is active
    */
   public boolean isActive()
   {
      return active;
   }

   /**
    * Set if action should be active
    */
   public void setActive(boolean active)
   {
      this.active = active;
   }   
}
