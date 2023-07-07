/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

/**
 * Collection of custom formatters for report fields
 */
public final class Formatter
{
   /**
    * Format period expressed in seconds as days/hours/minutes/seconds string.
    *
    * @param period period expressed in seconds
    * @return formatted string
    */
   public static final String secondsToString(int period)
   {
      return secondsToString((long)period);
   }

   /**
    * Format period expressed in seconds as days/hours/minutes/seconds string.
    *
    * @param period period expressed in seconds
    * @return formatted string
    */
   public static final String secondsToString(long period)
   {
      long days = period / 86400;
      long hours = (period % 86400) / 3600;
      long minutes = (period % 3600) / 60;
      long seconds = (period % 60);
      return String.format("%dd %02d:%02d:%02d", days, hours, minutes, seconds);
   }
}
