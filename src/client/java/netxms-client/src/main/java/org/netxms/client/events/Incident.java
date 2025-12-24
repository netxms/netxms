/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Incident - operational record for tracking and managing multiple alarms
 */
public class Incident
{
   // Incident states
   public static final int STATE_OPEN = 0;
   public static final int STATE_IN_PROGRESS = 1;
   public static final int STATE_PENDING = 2;
   public static final int STATE_RESOLVED = 3;
   public static final int STATE_CLOSED = 4;

   // Incident attributes
   private long id;
   private Date creationTime;
   private Date lastChangeTime;
   private int state;
   private int assignedUserId;
   private String title;
   private String description;
   private long sourceAlarmId;
   private long sourceObjectId;
   private long createdByUser;
   private long resolvedByUser;
   private long closedByUser;
   private Date resolveTime;
   private Date closeTime;
   private long[] linkedAlarmIds;
   private int commentsCount;

   /**
    * Create incident object from NXCP message
    *
    * @param msg Source NXCP message
    * @param baseId Base variable ID for list parsing
    */
   public Incident(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      creationTime = new Date(msg.getFieldAsInt64(baseId + 1) * 1000);
      lastChangeTime = new Date(msg.getFieldAsInt64(baseId + 2) * 1000);
      state = msg.getFieldAsInt32(baseId + 3);
      assignedUserId = msg.getFieldAsInt32(baseId + 4);
      title = msg.getFieldAsString(baseId + 5);
      description = msg.getFieldAsString(baseId + 6);
      sourceAlarmId = msg.getFieldAsInt64(baseId + 7);
      sourceObjectId = msg.getFieldAsInt64(baseId + 8);
      createdByUser = msg.getFieldAsInt64(baseId + 9);
      resolvedByUser = msg.getFieldAsInt64(baseId + 10);
      closedByUser = msg.getFieldAsInt64(baseId + 11);
      long rt = msg.getFieldAsInt64(baseId + 12);
      resolveTime = (rt > 0) ? new Date(rt * 1000) : null;
      long ct = msg.getFieldAsInt64(baseId + 13);
      closeTime = (ct > 0) ? new Date(ct * 1000) : null;
      commentsCount = msg.getFieldAsInt32(baseId + 14);

      // Linked alarm IDs are in a separate list
      int alarmCount = msg.getFieldAsInt32(baseId + 15);
      if (alarmCount > 0)
      {
         linkedAlarmIds = new long[alarmCount];
         for (int i = 0; i < alarmCount; i++)
         {
            linkedAlarmIds[i] = msg.getFieldAsInt64(baseId + 16 + i);
         }
      }
      else
      {
         linkedAlarmIds = new long[0];
      }
   }

   /**
    * Create incident object from NXCP message (details response)
    *
    * @param msg Source NXCP message
    */
   public Incident(NXCPMessage msg)
   {
      id = msg.getFieldAsInt64(NXCPCodes.VID_INCIDENT_ID);
      creationTime = new Date(msg.getFieldAsInt64(NXCPCodes.VID_CREATION_TIME) * 1000);
      lastChangeTime = new Date(msg.getFieldAsInt64(NXCPCodes.VID_LAST_CHANGE_TIME) * 1000);
      state = msg.getFieldAsInt32(NXCPCodes.VID_INCIDENT_STATE);
      assignedUserId = msg.getFieldAsInt32(NXCPCodes.VID_INCIDENT_ASSIGNED_USER);
      title = msg.getFieldAsString(NXCPCodes.VID_INCIDENT_TITLE);
      description = msg.getFieldAsString(NXCPCodes.VID_INCIDENT_DESCRIPTION);
      sourceAlarmId = msg.getFieldAsInt64(NXCPCodes.VID_SOURCE_ALARM_ID);
      sourceObjectId = msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID);
      createdByUser = msg.getFieldAsInt64(NXCPCodes.VID_USER_ID);
      resolvedByUser = msg.getFieldAsInt64(NXCPCodes.VID_RESOLVED_BY_USER);
      closedByUser = msg.getFieldAsInt64(NXCPCodes.VID_CLOSED_BY_USER);
      long rt = msg.getFieldAsInt64(NXCPCodes.VID_RESOLVE_TIME);
      resolveTime = (rt > 0) ? new Date(rt * 1000) : null;
      long ct = msg.getFieldAsInt64(NXCPCodes.VID_CLOSE_TIME);
      closeTime = (ct > 0) ? new Date(ct * 1000) : null;
      commentsCount = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COMMENTS);

      // Linked alarm IDs
      linkedAlarmIds = msg.getFieldAsUInt32Array(NXCPCodes.VID_INCIDENT_ALARM_LIST_BASE);
      if (linkedAlarmIds == null)
         linkedAlarmIds = new long[0];
   }

   /**
    * Get state name for the given state value
    *
    * @param state State value
    * @return State name
    */
   public static String getStateName(int state)
   {
      switch(state)
      {
         case STATE_OPEN:
            return "Open";
         case STATE_IN_PROGRESS:
            return "In Progress";
         case STATE_PENDING:
            return "Pending";
         case STATE_RESOLVED:
            return "Resolved";
         case STATE_CLOSED:
            return "Closed";
         default:
            return "Unknown";
      }
   }

   /**
    * Get incident ID
    *
    * @return incident ID
    */
   public long getId()
   {
      return id;
   }

   /**
    * Get creation time
    *
    * @return creation time
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * Get last change time
    *
    * @return last change time
    */
   public Date getLastChangeTime()
   {
      return lastChangeTime;
   }

   /**
    * Get incident state
    *
    * @return incident state
    */
   public int getState()
   {
      return state;
   }

   /**
    * Get state name for this incident
    *
    * @return state name
    */
   public String getStateName()
   {
      return getStateName(state);
   }

   /**
    * Get assigned user ID
    *
    * @return assigned user ID or 0 if not assigned
    */
   public int getAssignedUserId()
   {
      return assignedUserId;
   }

   /**
    * Get incident title
    *
    * @return incident title
    */
   public String getTitle()
   {
      return title;
   }

   /**
    * Get incident description
    *
    * @return incident description or null
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * Get source alarm ID
    *
    * @return source alarm ID or 0 if not created from alarm
    */
   public long getSourceAlarmId()
   {
      return sourceAlarmId;
   }

   /**
    * Get source object ID
    *
    * @return source object ID
    */
   public long getSourceObjectId()
   {
      return sourceObjectId;
   }

   /**
    * Get ID of user who created this incident
    *
    * @return creator user ID or 0 if auto-created
    */
   public long getCreatedByUser()
   {
      return createdByUser;
   }

   /**
    * Get ID of user who resolved this incident
    *
    * @return user ID or 0 if not resolved
    */
   public long getResolvedByUser()
   {
      return resolvedByUser;
   }

   /**
    * Get ID of user who closed this incident
    *
    * @return user ID or 0 if not closed
    */
   public long getClosedByUser()
   {
      return closedByUser;
   }

   /**
    * Get resolve time
    *
    * @return resolve time or null if not resolved
    */
   public Date getResolveTime()
   {
      return resolveTime;
   }

   /**
    * Get close time
    *
    * @return close time or null if not closed
    */
   public Date getCloseTime()
   {
      return closeTime;
   }

   /**
    * Get list of linked alarm IDs
    *
    * @return array of linked alarm IDs
    */
   public long[] getLinkedAlarmIds()
   {
      return linkedAlarmIds != null ? linkedAlarmIds : new long[0];
   }

   /**
    * Check if this incident has linked alarms
    *
    * @return true if incident has linked alarms
    */
   public boolean hasLinkedAlarms()
   {
      return linkedAlarmIds != null && linkedAlarmIds.length > 0;
   }

   /**
    * Get comments count
    *
    * @return comments count
    */
   public int getCommentsCount()
   {
      return commentsCount;
   }

   /**
    * Check if incident is in terminal state (Closed)
    *
    * @return true if incident is closed
    */
   public boolean isClosed()
   {
      return state == STATE_CLOSED;
   }

   /**
    * Check if incident is resolved
    *
    * @return true if incident is resolved or closed
    */
   public boolean isResolved()
   {
      return state == STATE_RESOLVED || state == STATE_CLOSED;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "Incident [id=" + id + ", creationTime=" + creationTime + ", lastChangeTime=" + lastChangeTime
            + ", state=" + getStateName() + ", assignedUserId=" + assignedUserId + ", title=" + title
            + ", sourceAlarmId=" + sourceAlarmId + ", sourceObjectId=" + sourceObjectId
            + ", createdByUser=" + createdByUser + ", resolvedByUser=" + resolvedByUser
            + ", closedByUser=" + closedByUser + ", resolveTime=" + resolveTime + ", closeTime=" + closeTime
            + ", linkedAlarmIds=" + Arrays.toString(linkedAlarmIds) + ", commentsCount=" + commentsCount + "]";
   }
}
