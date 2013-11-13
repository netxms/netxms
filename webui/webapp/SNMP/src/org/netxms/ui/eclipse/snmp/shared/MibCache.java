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
package org.netxms.ui.eclipse.snmp.shared;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URISyntaxException;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.osgi.service.datalocation.Location;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.MibTree;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.ui.eclipse.console.api.ConsoleLoginListener;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;

/**
 * SNMP MIB cache
 */
public final class MibCache implements ConsoleLoginListener
{	
	private static final Object MUTEX = new Object();
	
	private static MibTree mibTree = new MibTree();

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.console.api.ConsoleLoginListener#afterLogin(org.netxms.client.NXCSession, org.eclipse.swt.widgets.Display)
	 */
	@Override
	public void afterLogin(final NXCSession session, Display display)
	{
		ConsoleJob job = new ConsoleJob(Messages.get().LoginListener_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				Location loc = Platform.getInstanceLocation();
				if (loc != null)
				{
					File targetDir;
					try
					{
						targetDir = new File(loc.getURL().toURI());
					}
					catch(URISyntaxException e)
					{
						targetDir = new File(loc.getURL().getPath());
					}
					File mibFile = new File(targetDir, "netxms.mib"); //$NON-NLS-1$
					
					Date serverMibTimestamp = session.getMibFileTimestamp();
					if (!mibFile.exists() || (serverMibTimestamp.getTime() > mibFile.lastModified()))
					{
						File file = session.downloadMibFile();

						if (mibFile.exists())
							mibFile.delete();
						
						if (!file.renameTo(mibFile))
						{
							// Rename failed, try to copy file
							InputStream in = null;
							OutputStream out = null;
							try
							{
								in = new FileInputStream(file);
								out = new FileOutputStream(mibFile);
								byte[] buffer = new byte[16384];
						      int len;
						      while((len = in.read(buffer)) > 0)
						      	out.write(buffer, 0, len);
							}
							catch(Exception e)
							{
								throw e;
							}
							finally
							{
								if (in != null)
									in.close();
								if (out != null)
									out.close();
							}
					      
					      file.delete();
						}
					}
					
					synchronized(MUTEX)
					{
						MibCache.mibTree = new MibTree(mibFile);
					}
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().LoginListener_JobError;
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * @return the mibTree
	 */
	public static MibTree getMibTree()
	{
		synchronized(MUTEX)
		{
			return mibTree;
		}
	}
	
	/**
	 * Find matching object in tree. If exactMatch set to true, method will search for object with
	 * ID equal to given. If exactMatch set to false, and object with given id cannot be found, closest upper level
	 * object will be returned (i.e., if object .1.3.6.1.5 does not exist in the tree, but .1.3.6.1 does, .1.3.6.1 will
	 * be returned in search for .1.3.6.1.5).
	 * 
	 * @param oid object id to find
	 * @param exactMatch set to true if exact match required
	 * @return MIB object or null if matching object not found
	 */
	public static MibObject findObject(String oid, boolean exactMatch)
	{
		SnmpObjectId id;
		try
		{
			id = SnmpObjectId.parseSnmpObjectId(oid);
		}
		catch(SnmpObjectIdFormatException e)
		{
			return null;
		}
		synchronized(MUTEX)
		{
			return mibTree.findObject(id, exactMatch);
		}
	}
}
