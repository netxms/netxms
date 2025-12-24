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

import java.util.Date;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.IncidentState;

/**
 * Incident summary
 */
public class IncidentSummary
{
   private long id;
   private Date creationTime;
   private Date lastChangeTime;
   private IncidentState state;
   private int assignedUserId;
   private String title;
   private long sourceObjectId;
   private int alarmCount;

   /**
    * Create incident object from NXCP message
    *
    * @param msg Source NXCP message
    * @param baseId Base variable ID for list parsing
    */
   public IncidentSummary(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      creationTime = msg.getFieldAsDate(baseId + 1);
      lastChangeTime = msg.getFieldAsDate(baseId + 2);
      state = IncidentState.getByValue(msg.getFieldAsInt32(baseId + 3));
      assignedUserId = msg.getFieldAsInt32(baseId + 4);
      title = msg.getFieldAsString(baseId + 5);
      sourceObjectId = msg.getFieldAsInt64(baseId + 6);
      alarmCount = msg.getFieldAsInt32(baseId + 7);
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
    * Get source object ID
    *
    * @return source object ID
    */
   public long getSourceObjectId()
   {
      return sourceObjectId;
   }

   /**
    * Get number of linked alarms
    *
    * @return number of linked alarms
    */
   public int getAlarmCount()
   {
      return alarmCount;
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
      return "IncidentSummary [id=" + id + ", creationTime=" + creationTime + ", lastChangeTime=" + lastChangeTime + ", state=" + state + ", assignedUserId=" + assignedUserId + ", title=" + title +
            ", sourceObjectId=" + sourceObjectId + ", alarmCount=" + alarmCount + "]";
   }
}
