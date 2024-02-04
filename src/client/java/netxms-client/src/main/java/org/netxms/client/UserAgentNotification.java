/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.client;

import java.util.Arrays;
import java.util.Date;
import org.netxms.base.NXCPMessage;

/**
 * User support application notification
 */
public class UserAgentNotification
{
   private long id;
   private String message;
   private long[] objects;
   private String objectNames;
   private Date startTime;
   private Date endTime;
   private boolean recalled;
   private boolean onStartup;
   private Date creationTime; 
   private int createdBy;
   
   /**
    * Create notification object from NXCP message. 
    * 
    * @param msg NXCP message
    * @param baseId base field ID for this object
    * @param session associated client session
    */
   public UserAgentNotification(NXCPMessage msg, long baseId, NXCSession session)
   {      
      id = msg.getFieldAsInt32(baseId);
      message = msg.getFieldAsString(baseId + 1);
      startTime = msg.getFieldAsDate(baseId + 2);
      endTime = msg.getFieldAsDate(baseId + 3);
      onStartup = msg.getFieldAsBoolean(baseId + 4);
      objects = msg.getFieldAsUInt32Array(baseId + 5);

      Arrays.sort(objects); // FIXME: really needed here? Sort objects for comparator in table view
      StringBuilder sb = new StringBuilder();
      for(int i = 0; i < objects.length; i++)
      {
         if (i > 0)
            sb.append(", ");
         sb.append(session.getObjectName(objects[i]));
      }
      objectNames = sb.toString();

      recalled = msg.getFieldAsBoolean(baseId + 6);
      creationTime = msg.getFieldAsDate(baseId + 7);
      createdBy = msg.getFieldAsInt32(baseId + 8);
   }

   /**
    * Get notification ID.
    * 
    * @return notification ID
    */
   public long getId()
   {
      return id;
   }
   
   /**
    * Get notification message.
    * 
    * @return notification message
    */
   public String getMessage()
   {
      return message;
   }
   
   /**
    * Get list of object identifiers where this notification was sent.
    * 
    * @return list of object identifiers where this notification was sent
    */
   public long[] getObjects()
   {
      return objects;
   }
   
   /**
    * Get list of objects where this notification was sent as comma separated list.
    * 
    * @return list of objects where this notification was sent as comma separated list
    */
   public String getObjectNames()
   {
      return objectNames;
   }
   
   /**
    * Get notification start time.
    * 
    * @return notification start time
    */
   public Date getStartTime()
   {
      return startTime;
   }
   
   /**
    * Get notification end time.
    * 
    * @return notification end time
    */
   public Date getEndTime()
   {
      return endTime;
   }
   
   /**
    * Check if this notification was recalled.
    * 
    * @return true if this notification was recalled.
    */
   public boolean isRecalled()
   {
      return recalled;
   }
   
   /**
    * Check if thus notification should be shown every startup
    * 
    * @return true if notification should be shown every startup
    */
   public boolean isStartupNotification()
   {
      return onStartup;
   }

   /**
    * @return the creationTime
    */
   public Date getCreationTime()
   {
      return creationTime;
   }

   /**
    * @return the createdBy
    */
   public int getCreatedBy()
   {
      return createdBy;
   }
}
