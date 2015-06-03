/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
import org.netxms.base.Logger;

/**
 * Node poll types
 */
public enum NodePollType
{
   UNKNOWN(0),
   STATUS(1),
   CONFIGURATION_FULL(2),
   INTERFACES(3),
   TOPOLOGY(4),
   CONFIGURATION_NORMAL(5),
   INSTANCE_DISCOVERY(6);

   private int value;
   private static Map<Integer, NodePollType> lookupTable = new HashMap<Integer, NodePollType>();

   static
   {
      for(NodePollType element : NodePollType.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   /**
    * @param value
    */
   private NodePollType(int value)
   {
      this.value = value;
   }

   /**
    * @return
    */
   public int getValue()
   {
      return value;
   }

   /**
    * @param value
    * @return
    */
   public static NodePollType getByValue(int value)
   {
      final NodePollType element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(NodePollType.class.getName(), "Unknown element " + value);
         return UNKNOWN; // fallback
      }
      return element;
   }
}
