/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.resources;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.widgets.Display;

/**
 * Resource manager
 */
public final class ResourceManager
{
   /**
    * Prevent construction
    */
   private ResourceManager()
   {
   }

   /**
    * Get image descriptor from resources.
    * 
    * @param path image path
    * @return image descriptor
    */
   public static ImageDescriptor getImageDescriptor(String path)
   {
      return ImageDescriptor.createFromFile(ResourceManager.class, path.startsWith("/") ? path : "/" + path);
   }

   /**
    * Get image from resources. Image should be disposed by caller.
    * 
    * @param path image path
    * @return image object
    */
   public static Image getImage(String path)
   {
      ImageDescriptor d = getImageDescriptor(path);
      return (d != null) ? d.createImage() : null;
   }

   /**
    * Get image from resources. Image should be disposed by caller.
    * 
    * @param path image path
    * @return image object
    */
   public static Image getImage(Display display, String path)
   {
      ImageDescriptor d = getImageDescriptor(path);
      return (d != null) ? d.createImage(display) : null;
   }

   /**
    * Get image data from resources.
    * 
    * @param path image path
    * @return image data
    */
   public static ImageData getImageData(String path)
   {
      ImageDescriptor d = getImageDescriptor(path);
      return (d != null) ? d.getImageData(100) : null;
   }
}
