/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.xml.XmlDateConverter;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.convert.Convert;

@Root(strict = false)
public class TimePeriod
{
   @Element(required = false)
   private TimeFrameType timeFrameType;

   @Element(required = false)
   private int timeRange;

   @Element(required = false)
   private TimeUnit timeUnit;

   @Element(required = false)
   @Convert(XmlDateConverter.class)
   private Date timeFrom;

   @Element(required = false)
   @Convert(XmlDateConverter.class)
   private Date timeTo;

   /**
    * Create new time period.
    *
    * @param timeFrameType time frame type
    * @param timeRange time range length in time units
    * @param timeUnit time unit used for defining this range
    * @param timeFrom start point in time for fixed period, can be null for "back from now"
    * @param timeTo end point in time for fixed period, can be null for "back from now"
    */
   public TimePeriod(TimeFrameType timeFrameType, int timeRange, TimeUnit timeUnit, Date timeFrom, Date timeTo)
   {
      this.timeFrameType = timeFrameType;
      this.timeRange = timeRange;
      this.timeUnit = timeUnit;
      this.timeFrom = timeFrom;
      this.timeTo = timeTo;
   }
   
   /**
    * Default constructor. Created "back from now" type time period with 1 hour length.
    */
   public TimePeriod()
   {
      timeFrameType = TimeFrameType.BACK_FROM_NOW;
      timeRange = 1;
      timeUnit = TimeUnit.HOUR;
      timeTo = new Date();
      timeFrom = new Date(timeTo.getTime() - 3600000L);
   }

   /**
    * Copy constructor.
    * 
    * @param src time period to copy
    */
   public TimePeriod(TimePeriod src)
   {
      this.timeFrameType = src.timeFrameType;
      this.timeRange = src.timeRange;
      this.timeUnit = src.timeUnit;
      this.timeFrom = (src.timeFrom != null) ? new Date(src.timeFrom.getTime()) : null;
      this.timeTo = (src.timeTo != null) ? new Date(src.timeTo.getTime()) : null;
   }

   /**
    * Check if period time frame type is "back from now".
    *
    * @return true if period time frame type is "back from now"
    */
   public boolean isBackFromNow()
   {
      return timeFrameType == TimeFrameType.BACK_FROM_NOW;
   }

   /**
    * Get time frame type ("fixed" or "back from now") for this period.
    *
    * @return time frame type for this period
    */
   public TimeFrameType getTimeFrameType()
   {
      return timeFrameType;
   }

   /**
    * Set time frame type ("fixed" or "back from now") for this period.
    *
    * @param timeFrameType new time frame type for this period
    */
   public void setTimeFrameType(TimeFrameType timeFrameType)
   {
      this.timeFrameType = timeFrameType;
   }

   /**
    * Get time range in time units.
    *
    * @return time range in time units
    */
   public int getTimeRange()
   {
      return timeRange;
   }

   /**
    * Set time range in time units.
    *
    * @param timeRange new time range in time units
    */
   public void setTimeRange(int timeRange)
   {
      this.timeRange = timeRange;
   }

   /**
    * Get time unit used for defining this time range.
    *
    * @return time unit used for defining this time range.
    */
   public TimeUnit getTimeUnit()
   {
      return timeUnit;
   }

   /**
    * Set time unit used for defining this time range.
    *
    * @param timeUnit new time unit used for defining this time range.
    */
   public void setTimeUnit(TimeUnit timeUnit)
   {
      this.timeUnit = timeUnit;
   }

   /**
    * Get starting point in time for this time range.
    * 
    * @return starting point in time for this time range
    */
   public Date getTimeFrom()
   {
      return timeFrom;
   }

   /**
    * Set starting point in time for this time range.
    *
    * @param timeFrom new starting point in time for this time range
    */
   public void setTimeFrom(Date timeFrom)
   {
      this.timeFrom = timeFrom;
   }

   /**
    * Get ending point in time for this time range.
    * 
    * @return ending point in time for this time range
    */
   public Date getTimeTo()
   {
      return timeTo;
   }

   /**
    * Set ending point in time for this time range.
    *
    * @param timeTo new ending point in time for this time range
    */
   public void setTimeTo(Date timeTo)
   {
      this.timeTo = timeTo;
   }

   /**
    * Get period start calculated from time frame type, time unit, etc.
    *
    * @return calculated period start
    */
   public Date getPeriodStart()
   {
      if (isBackFromNow())
      {
         switch(timeUnit)
         {
            case MINUTE:
               return new Date(System.currentTimeMillis() - (long)timeRange * 60L * 1000L);
            case HOUR:
               return new Date(System.currentTimeMillis() - (long)timeRange * 60L * 60L * 1000L);
            case DAY:
               return new Date(System.currentTimeMillis() - (long)timeRange * 24L * 60L * 60L * 1000L);
         }
         return new Date();
      }
      else
      {
         return timeFrom;
      }
   }

   /**
    * Get period end calculated from time frame type, time unit, etc.
    *
    * @return calculated period end
    */
   public Date getPeriodEnd()
   {
      return isBackFromNow() ? new Date() : timeTo;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "TimePeriod [timeFrameType=" + timeFrameType + ", timeRange=" + timeRange + ", timeUnit=" + timeUnit + ", timeFrom="
            + timeFrom + ", timeTo=" + timeTo + "]";
   }
}
