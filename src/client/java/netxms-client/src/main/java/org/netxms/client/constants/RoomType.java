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
package org.netxms.client.constants;

import java.util.HashMap;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Room type
 */
public enum RoomType
{
   COMPUTER_ROOM(0),
   ELECTRICAL(1),
   MECHANICAL(2),
   TELECOM(3),
   OTHER(4);

   private static Logger logger = LoggerFactory.getLogger(RoomType.class);
   private static Map<Integer, RoomType> lookupTable = new HashMap<Integer, RoomType>();
   static
   {
      for(RoomType element : RoomType.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;

   /**
    * Internal constructor
    *
    * @param value integer value
    */
   private RoomType(int value)
   {
      this.value = value;
   }

   /**
    * Get integer value
    *
    * @return integer value
    */
   public int getValue()
   {
      return value;
   }

   /**
    * Get enum by integer value
    *
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static RoomType getByValue(int value)
   {
      final RoomType element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return OTHER; // fall-back
      }
      return element;
   }
}
