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
package org.netxms.client.objects.configs;

import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.RoomElementType;

/**
 * Passive room element (column, wall, ramp, stairs, door, etc.) placed on floor plan. All coordinates and dimensions are in
 * millimetres.
 */
public class RoomPassiveElement
{
   public long id;
   public String name;
   public RoomElementType type;
   public int x;
   public int y;
   public int rotation;
   public int width;
   public int depth;

   /**
    * Create empty element. ID 0 means that server will assign new ID when element is saved.
    */
   public RoomPassiveElement()
   {
      id = 0;
      name = "";
      type = RoomElementType.COLUMN;
      x = 0;
      y = 0;
      rotation = 0;
      width = 500;
      depth = 500;
   }

   /**
    * Create element from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public RoomPassiveElement(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId++);
      name = msg.getFieldAsString(baseId++);
      type = RoomElementType.getByValue(msg.getFieldAsInt32(baseId++));
      x = msg.getFieldAsInt32(baseId++);
      y = msg.getFieldAsInt32(baseId++);
      rotation = msg.getFieldAsInt32(baseId++);
      width = msg.getFieldAsInt32(baseId++);
      depth = msg.getFieldAsInt32(baseId++);
   }

   /**
    * Copy constructor
    *
    * @param src element to copy
    */
   public RoomPassiveElement(RoomPassiveElement src)
   {
      id = src.id;
      name = src.name;
      type = src.type;
      x = src.x;
      y = src.y;
      rotation = src.rotation;
      width = src.width;
      depth = src.depth;
   }

   /**
    * Fill message with element data
    *
    * @param msg message to fill
    * @param baseId base field ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setFieldInt32(baseId++, (int)id);
      msg.setField(baseId++, name);
      msg.setFieldInt16(baseId++, type.getValue());
      msg.setFieldInt32(baseId++, x);
      msg.setFieldInt32(baseId++, y);
      msg.setFieldInt32(baseId++, rotation);
      msg.setFieldInt32(baseId++, width);
      msg.setFieldInt32(baseId++, depth);
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "RoomPassiveElement [id=" + id + ", name=" + name + ", type=" + type + ", x=" + x + ", y=" + y + ", rotation=" + rotation + ", width=" + width + ", depth=" +
            depth + "]";
   }
}
