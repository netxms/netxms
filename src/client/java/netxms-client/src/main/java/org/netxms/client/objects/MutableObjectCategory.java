/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.util.UUID;

/**
 * Mutable variant of object category
 */
public class MutableObjectCategory extends ObjectCategory
{
   /**
    * Create new object category.
    *
    * @param name category name
    * @param icon UI icon for objects
    * @param mapImage default map image for objects
    */
   public MutableObjectCategory(String name, UUID icon, UUID mapImage)
   {
      super(name, icon, mapImage);
   }

   /**
    * Create mutable object from immutable.
    * 
    * @param src source object
    */
   public MutableObjectCategory(ObjectCategory src)
   {
      super(src);
   }

   /**
    * Set category name.
    *
    * @param name new category name
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * Set UI icon.
    * 
    * @param icon UUID of new UI icon
    */
   public void setIcon(UUID icon)
   {
      this.icon = icon;
   }

   /**
    * Set map image.
    * 
    * @param mapImage UUID of new map image
    */
   public void setMapImage(UUID mapImage)
   {
      this.mapImage = mapImage;
   }
}
