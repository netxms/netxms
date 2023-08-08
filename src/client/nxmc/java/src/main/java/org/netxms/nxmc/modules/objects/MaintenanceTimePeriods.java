/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects;

import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Maintenance time periods initializer
 */
public class MaintenanceTimePeriods
{
   private static Logger logger = LoggerFactory.getLogger(MaintenanceTimePeriods.class);
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 * @param session 
	 */
	public static void init(NXCSession session)
	{
      String line = session.getObjectMaintenancePredefinedPeriods();
      String[] entries = line.split(",");
      PreferenceStore ps = PreferenceStore.getInstance();
      for (String entrySrc : entries)
      {
         String entry = entrySrc.trim();
         int multiplier = 1;
         if (entry.toUpperCase().endsWith("M"))
            multiplier = 60;
         else if (entry.toUpperCase().endsWith("H"))
            multiplier = 60 * 60;
         else if (entry.toUpperCase().endsWith("D"))
            multiplier = 60 * 60  * 24;
         if (multiplier != 1)
         {
            entry = entry.substring(0, entry.length() - 1);
         }
         try
         {
            int time = Integer.parseInt(entry) * multiplier;
            int menuSize = ps.getAsInteger("Maintenance.TimeMenuSize", 0);
            boolean found = false;
            for(int i = 0; i < menuSize; i++)
            {
               final int currEntryTime = ps.getAsInteger("Maintenance.TimeMenuEntry." + Integer.toString(i), 0);
               if (time  == currEntryTime)
               {
                  found = true;
                  break;
               }
            }
            if (!found)
            {
               ps.set("Maintenance.TimeMenuEntry." + menuSize, time); 
               ps.set("Maintenance.TimeMenuSize", ++menuSize); 
            }
         }
         catch (Exception e)
         {
            logger.error(String.format("Error parsing time period defenition \"%s\"", entrySrc));
         }            
      }
	}
}
