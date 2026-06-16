/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2026 Raden Solutions
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
package org.netxms.client.constants;

import java.util.HashMap;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Calendar period for log time filters (resolved relative to current time in client time zone).
 */
public enum CalendarPeriod
{
   TODAY(0),
   YESTERDAY(1),
   THIS_WEEK(2),
   THIS_MONTH(3);

   private static Logger logger = LoggerFactory.getLogger(CalendarPeriod.class);
   private static Map<Integer, CalendarPeriod> lookupTable = new HashMap<Integer, CalendarPeriod>();
   static
   {
      for(CalendarPeriod element : CalendarPeriod.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;

   /**
    * Internal constructor
    *
    * @param value integer value
    */
   private CalendarPeriod(int value)
   {
      this.value = value;
   }

   /**
    * Get integer value
    *
    * @return integer value
    */
   public int getValue()
   {
      return value;
   }

   /**
    * Get enum element by integer value
    *
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static CalendarPeriod getByValue(int value)
   {
      final CalendarPeriod element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return TODAY; // fallback
      }
      return element;
   }
}
