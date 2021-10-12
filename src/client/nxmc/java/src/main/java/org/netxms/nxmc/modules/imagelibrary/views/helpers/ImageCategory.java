/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.imagelibrary.views.helpers;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import org.netxms.client.LibraryImage;

/**
 * Image category
 */
public class ImageCategory
{
   private String name;
   private Map<UUID, LibraryImage> images = new HashMap<UUID, LibraryImage>();
   
   public ImageCategory(String name)
   {
      this.name = name;
   }
   
   public String getName()
   {
      return name;
   }
   
   public void addImage(LibraryImage image)
   {
      images.put(image.getGuid(), image);
   }
   
   public boolean removeImage(UUID guid)
   {
      return images.remove(guid) != null;
   }
   
   public Collection<LibraryImage> getImages()
   {
      return images.values();
   }
   
   public boolean isEmpty()
   {
      return images.isEmpty();
   }
   
   public boolean hasProtectedImages()
   {
      for(LibraryImage i : images.values())
         if (i.isProtected())
            return true;
      return false;
   }
}
