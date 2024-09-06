/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objecttools;

import java.io.ByteArrayInputStream;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.ServiceLoader;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.services.ObjectToolHandler;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Cache for object tools
 */
public class ObjectToolsCache
{
   private static Logger logger = LoggerFactory.getLogger(ObjectToolsCache.class);
   private static Map<String, ObjectToolHandler> handlers = new HashMap<String, ObjectToolHandler>();
   
   private Map<Long, ObjectTool> objectTools = new HashMap<Long, ObjectTool>();
   private Map<Long, ImageDescriptor> icons = new HashMap<Long, ImageDescriptor>();
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
      ServiceLoader<ObjectToolHandler> loader = ServiceLoader.load(ObjectToolHandler.class, ObjectToolsCache.class.getClassLoader());
      for(ObjectToolHandler h : loader)
      {
         handlers.put(h.getId(), h);
         logger.debug("Registered object tool handler for ID=\"" + h.getId() + "\"");
      }
	}

	/**
	 * @param session
	 */
	private ObjectToolsCache(NXCSession session)
	{
	   this.session = session;

	   reload();

      session.addListener(new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case SessionNotification.OBJECT_TOOLS_CHANGED:
                  onObjectToolChange(n.getSubCode());
                  break;
               case SessionNotification.OBJECT_TOOL_DELETED:
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
      return Registry.getSingleton(ObjectToolsCache.class);
	}

   /**
    * Attach session to cache
    * 
    * @param session
    */
   public static void attachSession(Display display, NXCSession session)
   {
      ObjectToolsCache instance = new ObjectToolsCache(session);
      Registry.setSingleton(display, ObjectToolsCache.class, instance);
   }

	/**
	 * Reload object tools from server
	 */
   @SuppressWarnings("unused")
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
				   if (!Registry.IS_WEB_CLIENT || (tool.getToolType() != ObjectTool.TYPE_LOCAL_COMMAND)) 
				      objectTools.put(tool.getId(), tool);
				}
			}
			synchronized(icons)
         {
			   icons.clear();
			   
            for(ObjectTool tool : list)
            {
               byte[] imageBytes = tool.getImageData();
               if ((imageBytes == null) || (imageBytes.length == 0))
                  continue;
               
               ByteArrayInputStream input = new ByteArrayInputStream(imageBytes);
               try
               {
                  final ImageData imageData = new ImageData(input); 
                  icons.put(tool.getId(), WidgetHelper.createImageDescriptor(imageData));
               }
               catch(Exception e)
               {
                  logger.error(String.format("Exception in ObjectToolsCache.reload(): toolId=%d, toolName=%s", tool.getId(),
                        tool.getName()), e);
               }
            }
         }
		}
		catch(Exception e)
		{
         logger.error("Exception in ObjectToolsCache.reload()", e);
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
		synchronized(icons)
      {
		   icons.clear();
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
		synchronized(objectTools)
		{
			return objectTools.get(toolId);
		}
	}
	
	/**
	 * @param toolId
	 * @return
	 */
	public ImageDescriptor findIcon(long toolId)
	{
	   synchronized(icons)
      {
         return icons.get(toolId);
      }
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
