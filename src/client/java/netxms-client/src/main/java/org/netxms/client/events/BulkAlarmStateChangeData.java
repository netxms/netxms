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
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Data for bulk alarm state change
 */
public class BulkAlarmStateChangeData
{
   private List<Long> alarms;
   private int userId;
   private Date changeTime;

   /**
    * Create from NXCP message
    *
    * @param msg NXCP message
    */
   public BulkAlarmStateChangeData(NXCPMessage msg)
   {
      alarms = Arrays.asList(msg.getFieldAsUInt32ArrayEx(NXCPCodes.VID_ALARM_ID_LIST));
      userId = msg.getFieldAsInt32(NXCPCodes.VID_USER_ID);
      changeTime = msg.getFieldAsDate(NXCPCodes.VID_LAST_CHANGE_TIME);
   }

   /**
    * @return the alarms
    */
   public List<Long> getAlarms()
   {
      return alarms;
   }

   /**
    * @return the userId
    */
   public int getUserId()
   {
      return userId;
   }

   /**
    * @return the changeTime
    */
   public Date getChangeTime()
   {
      return changeTime;
   }
}
