/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
import java.util.Collections;
import java.util.Date;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.IncidentState;

/**
 * Incident - operational record for tracking and managing multiple alarms
 */
public class Incident
{
   private long id;
   private Date creationTime;
   private Date lastChangeTime;
   private IncidentState state;
   private int assignedUserId;
   private String title;
   private long sourceAlarmId;
   private long sourceObjectId;
   private int createdByUser;
   private int resolvedByUser;
   private int closedByUser;
   private Date resolveTime;
   private Date closeTime;
   private long[] linkedAlarmIds;
   private int commentsCount;
   private List<IncidentComment> comments;

   /**
    * Create incident object from NXCP message (details response)
    *
    * @param msg Source NXCP message
    */
   public Incident(NXCPMessage msg)
   {
      id = msg.getFieldAsInt64(NXCPCodes.VID_INCIDENT_ID);
      creationTime = msg.getFieldAsDate(NXCPCodes.VID_CREATION_TIME);
      lastChangeTime = msg.getFieldAsDate(NXCPCodes.VID_LAST_CHANGE_TIME);
      state = IncidentState.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_INCIDENT_STATE));
      assignedUserId = msg.getFieldAsInt32(NXCPCodes.VID_INCIDENT_ASSIGNED_USER);
      title = msg.getFieldAsString(NXCPCodes.VID_INCIDENT_TITLE);
      sourceAlarmId = msg.getFieldAsInt64(NXCPCodes.VID_SOURCE_ALARM_ID);
      sourceObjectId = msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID);
      createdByUser = msg.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      resolvedByUser = msg.getFieldAsInt32(NXCPCodes.VID_RESOLVED_BY_USER);
      closedByUser = msg.getFieldAsInt32(NXCPCodes.VID_CLOSED_BY_USER);
      long rt = msg.getFieldAsInt64(NXCPCodes.VID_RESOLVE_TIME);
      resolveTime = (rt > 0) ? new Date(rt * 1000) : null;
      long ct = msg.getFieldAsInt64(NXCPCodes.VID_CLOSE_TIME);
      closeTime = (ct > 0) ? new Date(ct * 1000) : null;
      commentsCount = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COMMENTS);

      // Linked alarm IDs
      linkedAlarmIds = msg.getFieldAsUInt32Array(NXCPCodes.VID_ALARM_ID_LIST);
      if (linkedAlarmIds == null)
         linkedAlarmIds = new long[0];

      // Parse comments
      comments = new ArrayList<>(commentsCount);
      long fieldId = NXCPCodes.VID_COMMENT_LIST_BASE;
      for (int i = 0; i < commentsCount; i++)
      {
         comments.add(new IncidentComment(msg, fieldId));
         fieldId += 10;
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
   public IncidentState getState()
   {
      return state;
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
   public int getCreatedByUser()
   {
      return createdByUser;
   }

   /**
    * Get ID of user who resolved this incident
    *
    * @return user ID or 0 if not resolved
    */
   public int getResolvedByUser()
   {
      return resolvedByUser;
   }

   /**
    * Get ID of user who closed this incident
    *
    * @return user ID or 0 if not closed
    */
   public int getClosedByUser()
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
    * Get comments
    *
    * @return list of comments
    */
   public List<IncidentComment> getComments()
   {
      return Collections.unmodifiableList(comments);
   }

   /**
    * Check if incident is in terminal state (Closed)
    *
    * @return true if incident is closed
    */
   public boolean isClosed()
   {
      return state == IncidentState.CLOSED;
   }

   /**
    * Check if incident is resolved
    *
    * @return true if incident is resolved or closed
    */
   public boolean isResolved()
   {
      return state == IncidentState.RESOLVED || state == IncidentState.CLOSED;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "Incident [id=" + id + ", creationTime=" + creationTime + ", lastChangeTime=" + lastChangeTime + ", state=" + state.name() + ", assignedUserId=" + assignedUserId + ", title=" + title +
            ", sourceAlarmId=" + sourceAlarmId + ", sourceObjectId=" + sourceObjectId + ", createdByUser=" + createdByUser + ", resolvedByUser=" + resolvedByUser + ", closedByUser=" + closedByUser +
            ", resolveTime=" + resolveTime + ", closeTime=" + closeTime + ", linkedAlarmIds=" + Arrays.toString(linkedAlarmIds) + ", commentsCount=" + commentsCount + "]";
   }
}
