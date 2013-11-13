/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolHandler;

/**
 * Cache for object tools
 */
public class ObjectToolsCache
{
   private static Map<String, ObjectToolHandler> handlers = new HashMap<String, ObjectToolHandler>();
   private static ObjectToolsCache instance = null;
   
   private Map<Long, ObjectTool> objectTools = new HashMap<Long, ObjectTool>();
	private NXCSession session = null;
	
	/**
	 * Initialize object tools cache. Should be called when connection with
	 * the server already established.
	 */
	public static void init()
	{
		registerHandlers();
	}
	
	/**
	 * Register object tool handlers
	 */
	private static void registerHandlers()
	{
		// Read all registered extensions and create tabs
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.objecttools.toolhandlers"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final ObjectToolHandler handler = (ObjectToolHandler)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				handlers.put(elements[i].getAttribute("id"), handler); //$NON-NLS-1$
			}
			catch(CoreException e)
			{
				e.printStackTrace();
				// TODO: add logging
			}
		}
	}
	
	/**
	 * @param session
	 */
	private ObjectToolsCache(NXCSession session)
	{
	   this.session = session;

	   reload();
	   
      session.addListener(new NXCListener() {
         @Override
         public void notificationHandler(SessionNotification n)
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
	 * Get cache instance
	 * 
	 * @return
	 */
	public static ObjectToolsCache getInstance()
	{
	   return instance;
	}

   /**
    * Attach session to cache
    * 
    * @param session
    */
   public static void attachSession(NXCSession session)
   {
      instance = new ObjectToolsCache(session);
   }
		
	/**
	 * Reload object tools from server
	 */
	private void reload()
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
	private void onObjectToolChange(final long toolId)
	{
		new Thread() {
			@Override
			public void run()
			{
				reload();
			}
		}.start();
	}

	/**
	 * Handler for object tool deletion
	 * 
	 * @param toolId ID of deleted tool
	 */
	private void onObjectToolDelete(final long toolId)
	{
		synchronized(objectTools)
		{
			objectTools.remove(toolId);
		}
	}
	
	/**
	 * Get current set of object tools. Returned array is a copy of
	 * cache content. 
	 * 
	 * @return current set of object tools
	 */
	public ObjectTool[] getTools()
	{
		ObjectTool[] tools = null;
		synchronized(objectTools)
		{
			tools = objectTools.values().toArray(new ObjectTool[objectTools.values().size()]);
		}
		return tools;
	}
	
	/**
	 * Find object tool in cache by ID
	 * 
	 * @param toolId tool id
	 * @return tool object or null if not found
	 */
	public ObjectTool findTool(long toolId)
	{
		ObjectTool tool;
		synchronized(objectTools)
		{
			tool = objectTools.get(toolId);
		}
		return tool;
	}
	
	/**
	 * Find handler for "internal" tool
	 * 
	 * @param toolId
	 * @return
	 */
	public static ObjectToolHandler findHandler(String toolId)
	{
		return handlers.get(toolId);
	}
}
