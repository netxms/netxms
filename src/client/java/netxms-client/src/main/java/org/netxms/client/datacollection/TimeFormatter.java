/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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

/**
 * Interface to format time
 */
public interface TimeFormatter
{
   /**
    * Format uptime number to human readable format
    * 
    * @param seconds number of seconds since uptime
    * @return uptime in human readable format
    */
   public String formatUptime(long seconds);

   /**
    * Format date and time UNIX timestamp to human readable format
    * 
    * @param timestamp UNIX timestamp to format 
    * @return time and date in human readable format
    */
   public String formatDateAndTime(long timestamp);
}
