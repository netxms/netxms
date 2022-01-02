/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.tools;

import java.util.Calendar;
import java.util.Date;

/**
 * Date builder - simple wrapper around calendar class intended for simplifying date building from calendar fields
 */
public class DateBuilder
{
   private Calendar calendar;

   /**
    * Create new date builder set to current time
    */
   public DateBuilder()
   {
      calendar = Calendar.getInstance();
   }

   /**
    * Add given amount to given calendar field.
    *
    * @param field calendar field
    * @param amount amount to add
    * @return this builder object
    */
   public DateBuilder add(int field, int amount)
   {
      calendar.add(field, amount);
      return this;
   }

   /**
    * Set given field to given value.
    *
    * @param field calendar field
    * @param value new value
    * @return this builder object
    */
   public DateBuilder set(int field, int value)
   {
      calendar.set(field, value);
      return this;
   }

   /**
    * Set time to midnight.
    *
    * @return this builder object
    */
   public DateBuilder setMidnight()
   {
      calendar.set(Calendar.HOUR, 0);
      calendar.set(Calendar.AM_PM, Calendar.AM);
      calendar.set(Calendar.MINUTE, 0);
      calendar.set(Calendar.SECOND, 0);
      return this;
   }

   /**
    * Set date to last day of currently selected month.
    *
    * @return this builder object
    */
   public DateBuilder setLastDayOfMonth()
   {
      calendar.set(Calendar.DAY_OF_MONTH, calendar.getActualMaximum(Calendar.DAY_OF_MONTH));
      return this;
   }

   /**
    * Create date object from this builder.
    *
    * @return date object corresponding to current builder settings
    */
   public Date create()
   {
      return calendar.getTime();
   }
}
