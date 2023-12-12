/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
 * Object poll types
 */
public enum ObjectPollType
{
   UNKNOWN(0),
   STATUS(1),
   CONFIGURATION_FULL(2),
   INTERFACES(3),
   TOPOLOGY(4),
   CONFIGURATION_NORMAL(5),
   INSTANCE_DISCOVERY(6),
   ROUTING_TABLE(7),
   NETWORK_DISCOVERY(8),
   AUTOBIND(9),
   MAP_UPDATE(10);

   private static Logger logger = LoggerFactory.getLogger(ObjectPollType.class);
   private static Map<Integer, ObjectPollType> lookupTable = new HashMap<Integer, ObjectPollType>();
   static
   {
      for(ObjectPollType element : ObjectPollType.values())
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
   private ObjectPollType(int value)
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
    * Get enum element by integer value
    * 
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static ObjectPollType getByValue(int value)
   {
      final ObjectPollType element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return UNKNOWN; // fallback
      }
      return element;
   }
}
