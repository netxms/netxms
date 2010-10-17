/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.snmp;

import java.io.File;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.osgi.service.datalocation.Location;
import org.eclipse.ui.IStartup;
import org.netxms.client.NXCSession;
import org.netxms.client.snmp.MibTree;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Early startup class
 *
 */
public class Startup implements IStartup
{
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IStartup#earlyStartup()
	 */
	@Override
	public void earlyStartup()
	{
		// wait for connect
		Job job = new Job("Load MIB file on startup") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				while(ConsoleSharedData.getInstance().getSession() == null)
				{
					try
					{
						Thread.sleep(1000);
					}
					catch(InterruptedException e)
					{
					}
				}
				try
				{
					NXCSession session = ConsoleSharedData.getInstance().getSession();
					Location loc = Platform.getInstanceLocation();
					if (loc != null)
					{
						File targetDir = new File(loc.getURL().toURI());
						File mibFile = new File(targetDir, "netxms.mib");

						if (!mibFile.exists())
						{
							File file = session.downloadMibFile();
							file.renameTo(mibFile);
						}
						
						MibTree mibTree = new MibTree(mibFile);
						Activator.setMibTree(mibTree);
					}
				}
				catch(Exception e)
				{
					e.printStackTrace();
				}
				return Status.OK_STATUS;
			}
		};
		job.setSystem(true);
		job.schedule();
	}
}
