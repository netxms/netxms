/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Cache for object tools
 */
public class ObjectToolsCache
{
   private static Logger logger = LoggerFactory.getLogger(ObjectToolsCache.class);

   private Map<Long, ObjectTool> objectTools = new HashMap<Long, ObjectTool>();
   private Map<Long, ImageDescriptor> icons = new HashMap<Long, ImageDescriptor>();
   private Map<UUID, Image> iconImages = new HashMap<UUID, Image>();
   private Display display = null;
	private NXCSession session = null;

	/**
	 * @param session
	 */
   private ObjectToolsCache(Display display, NXCSession session)
	{
      this.display = display;
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

      ImageProvider.getInstance().addUpdateListener(new ImageUpdateListener() {
         @Override
         public void imageUpdated(UUID guid)
         {
            onLibraryImageChange(guid);
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
      ObjectToolsCache instance = new ObjectToolsCache(display, session);
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

            ImageProvider imageProvider = ImageProvider.getInstance(display);
            for(ObjectTool tool : list)
            {
               final UUID iconGuid = tool.getIcon();
               if ((iconGuid == null) || iconGuid.equals(NXCommon.EMPTY_GUID))
                  continue;

               try
               {
                  imageProvider.preloadImageFromServer(iconGuid);
                  Image image = getIconImage(imageProvider, iconGuid);
                  if (image != null)
                     icons.put(tool.getId(), WidgetHelper.createImageDescriptor(image));
               }
               catch(Exception e)
               {
                  logger.error(String.format("Exception in ObjectToolsCache.reload(): toolId=%d, toolName=%s", tool.getId(), tool.getName()), e);
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
    * Get cache owned copy of object tool icon, creating it if necessary. Cache cannot use images provided by image provider
    * directly - they are held in LRU cache and can be disposed on eviction, while image descriptor created from image reads image
    * data from it lazily on every repaint at previously unseen zoom level. Must be called while holding lock on icons map.
    *
    * @param imageProvider image provider
    * @param guid library image GUID
    * @return cache owned copy of icon image or null if image is not available
    */
   private Image getIconImage(ImageProvider imageProvider, UUID guid)
   {
      Image image = iconImages.get(guid);
      if (image != null)
         return image;

      Image sourceImage = imageProvider.getObjectIcon(guid);
      if (sourceImage == null)
         return null;

      image = new Image(display, sourceImage, SWT.IMAGE_COPY);
      iconImages.put(guid, image);
      return image;
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
    * Handler for library image change. Updates cached icons for tools that use given image.
    *
    * @param guid library image GUID
    */
   private void onLibraryImageChange(UUID guid)
   {
      List<Long> affectedTools = new ArrayList<Long>();
      synchronized(objectTools)
      {
         for(ObjectTool tool : objectTools.values())
         {
            if (guid.equals(tool.getIcon()))
               affectedTools.add(tool.getId());
         }
      }
      if (affectedTools.isEmpty())
         return;

      ImageProvider imageProvider = ImageProvider.getInstance();
      synchronized(icons)
      {
         // Image content has changed, so cached copy has to be re-created. Old copy is intentionally not disposed here - image
         // descriptors already given to menus and actions read image data from it lazily on repaint. Copies left behind are 16x16
         // images and their number is limited by number of image library updates within single session.
         iconImages.remove(guid);

         ImageDescriptor icon = null;
         if (imageProvider.getLibraryImageObject(guid) != null)
         {
            Image image = getIconImage(imageProvider, guid);
            if (image != null)
               icon = WidgetHelper.createImageDescriptor(image);
         }

         for(Long toolId : affectedTools)
         {
            if (icon != null)
               icons.put(toolId, icon);
            else
               icons.remove(toolId);
         }
      }
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
	
}
