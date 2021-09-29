/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import java.util.Arrays;
import java.util.Date;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.Severity;

/**
 * Alarm
 */
public class Alarm
{
   // Alarm states
   public static final int STATE_OUTSTANDING = 0;
   public static final int STATE_ACKNOWLEDGED = 1;
   public static final int STATE_RESOLVED = 2;
   public static final int STATE_TERMINATED = 3;

   public static final int STATE_BIT_OUTSTANDING = 0x01;
   public static final int STATE_BIT_ACKNOWLEDGED = 0x02;
   public static final int STATE_BIT_RESOLVED = 0x04;
   public static final int STATE_BIT_TERMINATED = 0x08;

   public static final int STATE_MASK = 0x0F;

   // Alarm helpdesk states
   public static final int HELPDESK_STATE_IGNORED = 0;
   public static final int HELPDESK_STATE_OPEN = 1;
   public static final int HELPDESK_STATE_CLOSED = 2;

   // Alarm attributes
   private long id;
   private long parentId;
   private Severity currentSeverity;
   private Severity originalSeverity;
   private int repeatCount;
   private int state;
   private boolean sticky;
   private int acknowledgedByUser;
   private int resolvedByUser;
   private int terminatedByUser;
   private long sourceEventId;
   private int sourceEventCode;
   private long sourceObjectId;
   private long dciId;
   private Date creationTime;
   private Date lastChangeTime;
   private String message;
   private String key;
   private int helpdeskState;
   private String helpdeskReference;
   private int timeout;
   private int timeoutEvent;
   private int commentsCount;
   private int ackTime;
   private long[] categories;
   private long[] subordinateAlarms;
   private UUID ruleId;
   private String ruleDescription;

   /**
    * @param msg Source NXCP message
    */
   public Alarm(NXCPMessage msg)
   {
      id = msg.getFieldAsInt64(NXCPCodes.VID_ALARM_ID);
      parentId = msg.getFieldAsInt64(NXCPCodes.VID_PARENT_ALARM_ID);
      currentSeverity = Severity.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_CURRENT_SEVERITY));
      originalSeverity = Severity.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_ORIGINAL_SEVERITY));
      repeatCount = msg.getFieldAsInt32(NXCPCodes.VID_REPEAT_COUNT);
      state = msg.getFieldAsInt32(NXCPCodes.VID_STATE);
      sticky = msg.getFieldAsBoolean(NXCPCodes.VID_IS_STICKY);
      acknowledgedByUser = msg.getFieldAsInt32(NXCPCodes.VID_ACK_BY_USER);
      resolvedByUser = msg.getFieldAsInt32(NXCPCodes.VID_RESOLVED_BY_USER);
      terminatedByUser = msg.getFieldAsInt32(NXCPCodes.VID_TERMINATED_BY_USER);
      sourceEventId = msg.getFieldAsInt64(NXCPCodes.VID_EVENT_ID);
      sourceEventCode = msg.getFieldAsInt32(NXCPCodes.VID_EVENT_CODE);
      sourceObjectId = msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID);
      dciId = msg.getFieldAsInt64(NXCPCodes.VID_DCI_ID);
      creationTime = new Date(msg.getFieldAsInt64(NXCPCodes.VID_CREATION_TIME) * 1000);
      lastChangeTime = new Date(msg.getFieldAsInt64(NXCPCodes.VID_LAST_CHANGE_TIME) * 1000);
      message = msg.getFieldAsString(NXCPCodes.VID_ALARM_MESSAGE);
      key = msg.getFieldAsString(NXCPCodes.VID_ALARM_KEY);
      helpdeskState = msg.getFieldAsInt32(NXCPCodes.VID_HELPDESK_STATE);
      helpdeskReference = msg.getFieldAsString(NXCPCodes.VID_HELPDESK_REF);
      timeout = msg.getFieldAsInt32(NXCPCodes.VID_ALARM_TIMEOUT);
      timeoutEvent = msg.getFieldAsInt32(NXCPCodes.VID_ALARM_TIMEOUT_EVENT);
      commentsCount = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COMMENTS);
      ackTime = msg.getFieldAsInt32(NXCPCodes.VID_TIMESTAMP);
      categories = msg.getFieldAsUInt32Array(NXCPCodes.VID_CATEGORY_LIST);
      subordinateAlarms = msg.getFieldAsUInt32Array(NXCPCodes.VID_SUBORDINATE_ALARMS);
      ruleId = msg.getFieldAsUUID(NXCPCodes.VID_RULE_ID);
      ruleDescription = msg.getFieldAsString(NXCPCodes.VID_RULE_DESCRIPTION);
   }

   /**
    * Mark alarm as resolved. This call only updates local object state and do not change
    * actual alarm state on server. It can be used to update local alarm objects after
    * receiving bulk alarm state change notification.
    *
    * @param userId ID of user that resolve this alarm
    * @param changeTime time when alarm was resolved
    */
   public void setResolved(int userId, Date changeTime)
   {
      state = STATE_RESOLVED;
      lastChangeTime = changeTime;
      resolvedByUser = userId;
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return the parentId
    */
   public long getParentId()
   {
      return parentId;
   }

   /**
    * @return the currentSeverity
    */
   public Severity getCurrentSeverity()
   {
      return currentSeverity;
   }

   /**
    * @return the originalSeverity
    */
   public Severity getOriginalSeverity()
   {
      return originalSeverity;
   }

   /**
    * @return the repeatCount
    */
   public int getRepeatCount()
   {
      return repeatCount;
   }

   /**
    * Get alarm state.
    *
    * @return alarm state
    */
   public int getState()
   {
      return state;
   }

   /**
    * Get alarm state bit. It can be used to match against state filter bit mask.
    *
    * @return alarm state bit
    */
   public int getStateBit()
   {
      switch(state)
      {
         case STATE_ACKNOWLEDGED:
            return STATE_BIT_ACKNOWLEDGED;
         case STATE_OUTSTANDING:
            return STATE_BIT_OUTSTANDING;
         case STATE_RESOLVED:
            return STATE_BIT_RESOLVED;
         case STATE_TERMINATED:
            return STATE_BIT_TERMINATED;
         default:
            return 0;
      }
   }

   /**
    * Get id of user that acknowledged this alarm (or 0 if alarm is not acknowledged).
    *
    * @return id of user that acknowledged this alarm (or 0 if alarm is not acknowledged)
    */
   public int getAcknowledgedByUser()
   {
      return acknowledgedByUser;
   }

   /**
    * Get id of user that terminated this alarm (or 0 if alarm is not terminated).
    *
    * @return id of user that terminated this alarm (or 0 if alarm is not terminated)
    */
   public int getTerminatedByUser()
   {
      return terminatedByUser;
   }

   /**
    * @return the sourceEventId
    */
   public long getSourceEventId()
   {
      return sourceEventId;
   }

   /**
    * @return the sourceEventCode
    */
   public int getSourceEventCode()
   {
      return sourceEventCode;
   }

   /**
    * @return the sourceObjectId
    */
   public long getSourceObjectId()
   {
      return sourceObjectId;
   }

   /**
    * @return the dciId
    */
   public long getDciId()
   {
      return dciId;
   }

   /**
    * @return the creationTime
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return the lastChangeTime
    */
   public Date getLastChangeTime()
   {
      return lastChangeTime;
   }

   /**
    * @return the message
    */
   public String getMessage()
   {
      return message;
   }

   /**
    * @return the key
    */
   public String getKey()
   {
      return key;
   }

   /**
    * @return the helpdeskState
    */
   public int getHelpdeskState()
   {
      return helpdeskState;
   }

   /**
    * @return the helpdeskReference
    */
   public String getHelpdeskReference()
   {
      return helpdeskReference;
   }

   /**
    * @return the timeout
    */
   public int getTimeout()
   {
      return timeout;
   }

   /**
    * @return the timeoutEvent
    */
   public int getTimeoutEvent()
   {
      return timeoutEvent;
   }

   /**
    * @return the commentsCount
    */
   public int getCommentsCount()
   {
      return commentsCount;
   }

   /**
    * Get id of user that resolved this alarm (or 0 if alarm is not resolved).
    *
    * @return id of user that resolved this alarm (or 0 if alarm is not resolved)
    */
   public int getResolvedByUser()
   {
      return resolvedByUser;
   }

   /**
    * @return the sticky
    */
   public boolean isSticky()
   {
      return sticky;
   }

   /**
    * Get time when alarm was acknowledged
    *
    * @return time when alarm was acknowledged
    */
   public int getAckTime()
   {
      return ackTime;
   }

   /**
    * Get list of categories this alarm belongs to.
    *
    * @return list of categories this alarm belongs to
    */
   public long[] getCategories()
   {
      return (categories != null) ? categories : new long[0];
   }

   /**
    * Get list of subordinate alarm identifiers.
    *
    * @return list of subordinate alarm identifiers
    */
   public long[] getSubordinateAlarms()
   {
      return (subordinateAlarms != null) ? subordinateAlarms : new long[0];
   }
   
   /**
    * Check if this alarm has any subordinated alarms.
    * 
    * @return true if this alarm has any subordinated alarms
    */
   public boolean hasSubordinatedAlarms()
   {
      return (subordinateAlarms != null) && (subordinateAlarms.length > 0);
   }

   /**
    * Get unique ID of Event Processing Policy rule that caused the alarm
    *
    * @return the rule UUID
    */
   public UUID getRuleId()
   {
      return ruleId;
   }

   /**
    * Get description of Event Processing Policy rule that caused the alarm
    *
    * @return the rule Description
    */
   public String getRuleDescription()
   {
      return ruleDescription;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "Alarm [id=" + id + ", parentId=" + parentId + ", currentSeverity=" + currentSeverity + ", originalSeverity="
            + originalSeverity + ", repeatCount=" + repeatCount + ", state=" + state + ", sticky=" + sticky
            + ", acknowledgedByUser=" + acknowledgedByUser + ", resolvedByUser=" + resolvedByUser + ", terminatedByUser="
            + terminatedByUser + ", sourceEventId=" + sourceEventId + ", sourceEventCode=" + sourceEventCode + ", sourceObjectId="
            + sourceObjectId + ", dciId=" + dciId + ", creationTime=" + creationTime + ", lastChangeTime=" + lastChangeTime
            + ", message=" + message + ", key=" + key + ", helpdeskState=" + helpdeskState + ", helpdeskReference="
            + helpdeskReference + ", timeout=" + timeout + ", timeoutEvent=" + timeoutEvent + ", commentsCount=" + commentsCount
            + ", ackTime=" + ackTime + ", categories=" + Arrays.toString(categories) + ", subordinateAlarms="
            + Arrays.toString(subordinateAlarms) + ", eppGuid=" + ruleId + ", eppDescription=" + ruleDescription + "]";
   }
}
