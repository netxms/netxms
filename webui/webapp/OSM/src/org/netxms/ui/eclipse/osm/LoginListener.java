/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.osm;

import java.io.File;
import java.net.URISyntaxException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.osgi.service.datalocation.Location;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.api.ConsoleLoginListener;

/**
 * Early startup handler
 */
public class LoginListener implements ConsoleLoginListener
{
	/**
	 * @see org.netxms.ui.eclipse.console.api.ConsoleLoginListener#afterLogin(org.netxms.client.NXCSession, org.eclipse.swt.widgets.Display)
	 */
	@Override
	public void afterLogin(NXCSession session, Display display)
	{
		GeoLocationCache.getInstance(display).initialize(session);
		Job housekeeper = new Job("Tile housekeeper") {
         @Override
         protected IStatus run(IProgressMonitor monitor)
         {
            Location loc = Platform.getInstanceLocation();
            File targetDir;
            try
            {
               targetDir = new File(loc.getURL().toURI());
            }
            catch(URISyntaxException e)
            {
               targetDir = new File(loc.getURL().getPath());
            }
            File base = new File(targetDir, "OSM");
            if (base.isDirectory())
            {
               cleanTileFiles(base.listFiles());
            }
            schedule(3600000L);
            return Status.OK_STATUS;
         }
      };
      housekeeper.setUser(false);
      housekeeper.setSystem(true);
      housekeeper.schedule();
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
	         Activator.log("Deleting expired tile file " + f.getAbsolutePath());
	         f.delete();
	      }
	   }
	}
}
