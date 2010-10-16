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
package org.netxms.ui.eclipse.objecttools;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.netxms.api.client.ISessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * Cache for object tools
 *
 */
public class ObjectToolsCache
{
	private static Map<Long, ObjectTool> objectTools = new HashMap<Long, ObjectTool>();
	private static NXCSession session = null;
	
	/**
	 * Initialize object tools cache. Should be called when connection with
	 * the server already established.
	 */
	public static void init()
	{
		session = NXMCSharedData.getInstance().getSession();
		
		reload();
		
		session.addListener(new NXCListener() {
			@Override
			public void notificationHandler(ISessionNotification n)
			{
				switch(n.getCode())
				{
					case NXCNotification.OBJECT_TOOLS_CHANGED:
						onObjectToolChange(n.getSubCode());
						break;
					case NXCNotification.OBJECT_TOOL_DELETED:
						onObjectToolDelete(n.getSubCode());
						break;
				}
			}
		});
	}

	/**
	 * Reload object tools from server
	 */
	private static void reload()
	{
		try
		{
			List<ObjectTool> list = session.getObjectTools();
			synchronized(objectTools)
			{
				objectTools.clear();
				for(ObjectTool tool : list)
				{
					objectTools.put(tool.getId(), tool);
				}
			}
		}
		catch(Exception e)
		{
			e.printStackTrace();
			// TODO: add logging
		}
	}
	
	/**
	 * Handler for object tool change
	 * 
	 * @param toolId ID of changed tool
	 */
	private static void onObjectToolChange(final long toolId)
	{
		Job job = new Job("Update object tools cache") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				// TODO: replace full reload with sending tool data
				//       in update message
				reload();
				return Status.OK_STATUS;
			}
		};
		job.setUser(false);
		job.schedule();
	}

	/**
	 * Handler for object tool deletion
	 * 
	 * @param toolId ID of deleted tool
	 */
	private static void onObjectToolDelete(final long toolId)
	{
		synchronized(objectTools)
		{
			objectTools.remove(toolId);
		}
	}
	
	/**
	 * Get current set of object tools
	 * 
	 * @return current set of object tools
	 */
	public static ObjectTool[] getTools()
	{
		ObjectTool[] tools = null;
		synchronized(objectTools)
		{
			tools = objectTools.values().toArray(new ObjectTool[objectTools.values().size()]);
		}
		return tools;
	}
}
