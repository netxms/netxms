/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.reporting.tools;

import java.util.Calendar;
import java.util.Date;

/**
 * Parser for report parameter of type "date"
 */
public class DateParameterParser
{
   private static final int[] ELEMENT_TYPES = { Calendar.YEAR, Calendar.MONTH, Calendar.DAY_OF_MONTH };

   /**
    * Get date and time from given text value.
    *
    * @param textValue parameter's text value
    * @param endOfDay end of day indicator - if true, time part will be set to 23:59:59, otherwise to 00:00:00
    * @return date object
    */
   public static Date getDateTime(String textValue, boolean endOfDay)
   {
      if (textValue.isEmpty())
         return null;

      String[] elements = textValue.split(";");
      if (elements.length != 3) // format - year;month;day
         return null;

      Calendar calendar = Calendar.getInstance();
      for(int i = 0; i < elements.length; i++)
      {
         parseElement(calendar, elements[i], ELEMENT_TYPES[i]);
      }

      if (endOfDay)
      {
         calendar.set(Calendar.HOUR_OF_DAY, 23);
         calendar.set(Calendar.MINUTE, 59);
         calendar.set(Calendar.SECOND, 59);
      }
      else
      {
         calendar.set(Calendar.HOUR_OF_DAY, 0);
         calendar.set(Calendar.MINUTE, 0);
         calendar.set(Calendar.SECOND, 0);
      }

      return calendar.getTime();
   }

   /**
    * Parse date element and adjust calendar accordingly
    *
    * @param calendar calendar to update
    * @param value element value
    * @param type element type (year, month, or day)
    */
   private static void parseElement(Calendar calendar, String value, int type)
   {
      // Try to interpret value as integer
      try
      {
         int numericValue = Integer.valueOf(value.trim());
         calendar.set(type, (type == Calendar.MONTH ? numericValue - 1 : numericValue));
         return;
      }
      catch(NumberFormatException e)
      {
      }

      // Check if there is offset from special value (like "last-1")
      int offset = 0;
      int index = value.indexOf('+');
      if (index == -1)
         index = value.indexOf('-');
      if (index != -1)
      {
         try
         {
            offset = Integer.parseInt(value.substring(index + 1));
            if (value.charAt(index) == '-')
               offset = -offset;
         }
         catch(NumberFormatException e)
         {
         }
         value = value.substring(0, index).trim();
      }
      else
      {
         value = value.trim();
      }

      if (value.equalsIgnoreCase("first"))
      {
         calendar.set(type, calendar.getActualMinimum(type));
      }
      else if (value.equalsIgnoreCase("last"))
      {
         calendar.set(type, calendar.getActualMaximum(type));
      }
      else if (value.equalsIgnoreCase("previous"))
      {
         calendar.add(type, -1);
      }
      else if (value.equalsIgnoreCase("next"))
      {
         calendar.add(type, 1);
      }

      calendar.add(type, offset);
   }
}
