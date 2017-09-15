/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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

import java.util.Date;
import org.netxms.client.datacollection.GraphSettings;

public class TimePeriod
{
   private int timeFrameType; 
   private int timeRangeValue;
   private int timeUnitValue; 
   private Date timeFromValue;
   private Date timeToValue;
   
   public TimePeriod(int timeFrameType, int timeRangeValue, int timeUnitValue, Date timeFromValue, Date timeToValue)
   {
      this.timeFrameType = timeFrameType;
      this.timeRangeValue = timeRangeValue;
      this.timeUnitValue = timeUnitValue;
      this.timeFromValue = timeFromValue;
      this.timeToValue = timeToValue;
   }
   
   public TimePeriod()
   {
      timeFrameType = GraphSettings.TIME_FRAME_BACK_FROM_NOW;
      timeRangeValue = 2;
      timeUnitValue = GraphSettings.TIME_UNIT_HOUR;      
   }
   
   public boolean isBackFromNow()
   {
      return timeFrameType == GraphSettings.TIME_FRAME_BACK_FROM_NOW;
   }
   
   /**
    * @return the timeFrameType
    */
   public int getTimeFrameType()
   {
      return timeFrameType;
   }
   /**
    * @param timeFrameType the timeFrameType to set
    */
   public void setTimeFrameType(int timeFrameType)
   {
      this.timeFrameType = timeFrameType;
   }
   /**
    * @return the timeRangeValue
    */
   public int getTimeRangeValue()
   {
      return timeRangeValue;
   }
   /**
    * @param timeRangeValue the timeRangeValue to set
    */
   public void setTimeRangeValue(int timeRangeValue)
   {
      this.timeRangeValue = timeRangeValue;
   }
   /**
    * @return the timeUnitValue
    */
   public int getTimeUnitValue()
   {
      return timeUnitValue;
   }
   /**
    * @param timeUnitValue the timeUnitValue to set
    */
   public void setTimeUnitValue(int timeUnitValue)
   {
      this.timeUnitValue = timeUnitValue;
   }
   /**
    * @return the timeFromValue
    */
   public Date getTimeFromValue()
   {
      return timeFromValue;
   }
   /**
    * @param timeFromValue the timeFromValue to set
    */
   public void setTimeFromValue(Date timeFromValue)
   {
      this.timeFromValue = timeFromValue;
   }
   /**
    * @return the timeToValue
    */
   public Date getTimeToValue()
   {
      return timeToValue;
   }
   /**
    * @param timeToValue the timeToValue to set
    */
   public void setTimeToValue(Date timeToValue)
   {
      this.timeToValue = timeToValue;
   }
   
   /**
    * @return Start date of request period
    */
   public Date getPeriodStart()
   {
      if(isBackFromNow())
      {
         switch(timeUnitValue)
         {
            case GraphSettings.TIME_UNIT_MINUTE:
               return new Date((new Date()).getTime() - (long)timeRangeValue * 60L * 1000L);
            case GraphSettings.TIME_UNIT_HOUR:
               return new Date((new Date()).getTime() - (long)timeRangeValue * 60L * 60L * 1000L);
            case GraphSettings.TIME_UNIT_DAY:
               return new Date((new Date()).getTime() - (long)timeRangeValue * 24L * 60L * 60L * 1000L);
         }
         return new Date();
      }
      else
      {
         return getTimeFromValue();
      }
   }
   
   /**
    * @return end date for request period
    */
   public Date getPeriodEnd()
   {
      if(isBackFromNow())
      {
         return new Date();
      }
      else
      {
         return getTimeToValue();
      }
   }
}
