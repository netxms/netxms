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
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;

/**
 * Object category
 */
public class ObjectCategory
{
   protected int id;
   protected String name;
   protected UUID icon;
   protected UUID mapImage;

   /**
    * Create new category object. Intended for subclasses only.
    *
    * @param name category name
    * @param icon UI icon for objects
    * @param mapImage default map image for objects
    */
   protected ObjectCategory(String name, UUID icon, UUID mapImage)
   {
      id = 0;
      this.name = name;
      this.icon = icon;
      this.mapImage = mapImage;
   }

   /**
    * Create category object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public ObjectCategory(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      name = msg.getFieldAsString(baseId + 1);
      icon = msg.getFieldAsUUID(baseId + 2);
      mapImage = msg.getFieldAsUUID(baseId + 3);
   }

   /**
    * Create copy of given category object
    *
    * @param src source object
    */
   public ObjectCategory(ObjectCategory src)
   {
      id = src.id;
      name = src.name;
      icon = src.icon;
      mapImage = src.mapImage;
   }

   /**
    * Fill NXCP message with category data.
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_CATEGORY_ID, id);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_ICON, icon);
      msg.setField(NXCPCodes.VID_IMAGE, mapImage);
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the icon
    */
   public UUID getIcon()
   {
      return !NXCommon.EMPTY_GUID.equals(icon) ? icon : null;
   }

   /**
    * @return the mapImage
    */
   public UUID getMapImage()
   {
      return !NXCommon.EMPTY_GUID.equals(mapImage) ? mapImage : null;
   }

   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }
}
