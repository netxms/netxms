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

/**
 * Incident activity log entry
 */
public class IncidentActivity
{
   // Activity types
   public static final int ACTIVITY_CREATED = 0;
   public static final int ACTIVITY_STATE_CHANGE = 1;
   public static final int ACTIVITY_ASSIGNED = 2;
   public static final int ACTIVITY_ALARM_LINKED = 3;
   public static final int ACTIVITY_ALARM_UNLINKED = 4;
   public static final int ACTIVITY_COMMENT_ADDED = 5;
   public static final int ACTIVITY_UPDATED = 6;

   private long id;
   private long incidentId;
   private Date timestamp;
   private long userId;
   private int activityType;
   private String oldValue;
   private String newValue;
   private String details;

   /**
    * Create incident activity entry from NXCP message
    *
    * @param msg Source NXCP message
    * @param baseId Base variable ID for parsing
    */
   public IncidentActivity(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      incidentId = msg.getFieldAsInt64(baseId + 1);
      timestamp = new Date(msg.getFieldAsInt64(baseId + 2) * 1000);
      userId = msg.getFieldAsInt64(baseId + 3);
      activityType = msg.getFieldAsInt32(baseId + 4);
      oldValue = msg.getFieldAsString(baseId + 5);
      newValue = msg.getFieldAsString(baseId + 6);
      details = msg.getFieldAsString(baseId + 7);
   }

   /**
    * Get activity type name
    *
    * @param type Activity type
    * @return Activity type name
    */
   public static String getActivityTypeName(int type)
   {
      switch(type)
      {
         case ACTIVITY_CREATED:
            return "Created";
         case ACTIVITY_STATE_CHANGE:
            return "State Changed";
         case ACTIVITY_ASSIGNED:
            return "Assigned";
         case ACTIVITY_ALARM_LINKED:
            return "Alarm Linked";
         case ACTIVITY_ALARM_UNLINKED:
            return "Alarm Unlinked";
         case ACTIVITY_COMMENT_ADDED:
            return "Comment Added";
         case ACTIVITY_UPDATED:
            return "Updated";
         default:
            return "Unknown";
      }
   }

   /**
    * Get activity entry ID
    *
    * @return activity entry ID
    */
   public long getId()
   {
      return id;
   }

   /**
    * Get incident ID
    *
    * @return incident ID
    */
   public long getIncidentId()
   {
      return incidentId;
   }

   /**
    * Get activity timestamp
    *
    * @return activity timestamp
    */
   public Date getTimestamp()
   {
      return timestamp;
   }

   /**
    * Get user ID who performed the action
    *
    * @return user ID
    */
   public long getUserId()
   {
      return userId;
   }

   /**
    * Get activity type
    *
    * @return activity type
    */
   public int getActivityType()
   {
      return activityType;
   }

   /**
    * Get activity type name
    *
    * @return activity type name
    */
   public String getActivityTypeName()
   {
      return getActivityTypeName(activityType);
   }

   /**
    * Get old value (for state changes, etc.)
    *
    * @return old value or null
    */
   public String getOldValue()
   {
      return oldValue;
   }

   /**
    * Get new value (for state changes, etc.)
    *
    * @return new value or null
    */
   public String getNewValue()
   {
      return newValue;
   }

   /**
    * Get activity details
    *
    * @return activity details or null
    */
   public String getDetails()
   {
      return details;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "IncidentActivity [id=" + id + ", incidentId=" + incidentId + ", timestamp=" + timestamp
            + ", userId=" + userId + ", activityType=" + getActivityTypeName()
            + ", oldValue=" + oldValue + ", newValue=" + newValue + ", details=" + details + "]";
   }
}
