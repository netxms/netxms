/**
 * NetXMS - open source network management system
 * Copyright (C) 2022-2026 Raden Solutions
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
package org.netxms.client.datacollection;

import java.util.function.LongFunction;

/**
 * Interface to format time
 */
public interface TimeFormatter
{
   /**
    * Format time span in seconds to human readable format
    *
    * @param seconds time span in seconds
    * @return time span in human readable format
    */
   public String formatTimeSpan(long seconds);

   /**
    * Format date and time UNIX timestamp to human readable format
    *
    * @param timestamp UNIX timestamp to format
    * @return time and date in human readable format
    */
   public String formatDateAndTime(long timestamp);

   /**
    * Format time span as [-][n days, ]hh:mm[:ss]. When seconds are hidden, round to the nearest minute,
    * with exact halves rounded away from zero.
    *
    * @param seconds time span in seconds
    * @param showSeconds true to show seconds
    * @param dayText provides localized day part ("1 day", "2 days") for given number of days
    * @return formatted time span
    */
   public static String formatTimeSpan(long seconds, boolean showSeconds, LongFunction<String> dayText)
   {
      StringBuilder sb = new StringBuilder();
      if (seconds < 0)
      {
         sb.append('-');
         seconds = -seconds;
      }
      if (!showSeconds)
         seconds = (seconds + 30) / 60 * 60;

      long days = seconds / 86400;
      if (days > 0)
      {
         sb.append(dayText.apply(days));
         sb.append(", ");
         seconds -= days * 86400;
      }

      long hours = seconds / 3600;
      if (hours < 10)
         sb.append('0');
      sb.append(hours);
      seconds -= hours * 3600;

      sb.append(':');
      long minutes = seconds / 60;
      if (minutes < 10)
         sb.append('0');
      sb.append(minutes);

      if (showSeconds)
      {
         sb.append(':');
         seconds = seconds % 60;
         if (seconds < 10)
            sb.append('0');
         sb.append(seconds);
      }

      return sb.toString();
   }
}
