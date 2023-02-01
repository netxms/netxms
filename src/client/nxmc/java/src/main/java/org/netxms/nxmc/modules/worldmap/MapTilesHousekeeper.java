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
package org.netxms.nxmc.modules.worldmap;

import java.io.File;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.services.LoginListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Map tiles housekeeper
 */
public class MapTilesHousekeeper implements LoginListener
{
   private static final Logger logger = LoggerFactory.getLogger(MapTilesHousekeeper.class);

	/**
    * @see org.netxms.nxmc.services.LoginListener#afterLogin(org.netxms.client.NXCSession, org.eclipse.swt.widgets.Display)
    */
	@Override
	public void afterLogin(NXCSession session, Display display)
	{
		GeoLocationCache.attachSession(display, session);
      Job housekeeper = new Job("Map tiles housekeeper", null, null, display) {
         @Override
         protected void run(IProgressMonitor monitor)
         {
            if (display.isDisposed())
            {
               logger.info("Map tiles housekeeper thread terminated because display is disposed");
               return;
            }

            try
            {
               File base = new File(Registry.getStateDir(getDisplay()), "MapTiles");
               if (base.isDirectory())
               {
                  cleanTileFiles(base.listFiles());
               }
            }
            catch(Exception e)
            {
               logger.error("Exception in tile housekeeper", e);
            }
            schedule(3600000L);
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      };
      housekeeper.setUser(false);
      housekeeper.setSystem(true);
      housekeeper.schedule(600000L);
      logger.info("Map tiles housekeeper job scheduled");
	}

	/**
	 * Clean expired tile files
	 * 
	 * @param files files and directories on current level
	 */
	private static void cleanTileFiles(File[] files)
	{
	   long now = System.currentTimeMillis();
	   for(File f : files)
	   {
	      if (f.isDirectory())
	      {
	         cleanTileFiles(f.listFiles());
	      }
	      else if (now - f.lastModified() > 7 * 86400 * 1000)
	      {
            logger.info("Deleting expired tile file " + f.getAbsolutePath());
	         f.delete();
	      }
	   }
	}
}
