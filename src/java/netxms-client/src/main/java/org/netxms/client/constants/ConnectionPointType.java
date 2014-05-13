/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
 * Access point state
 */
public enum ConnectionPointType
{
   INDIRECT(0),
   DIRECT(1),
   WIRELESS(2);

   private int value;
   private static Map<Integer, ConnectionPointType> lookupTable = new HashMap<Integer, ConnectionPointType>();

   static
   {
      for(ConnectionPointType element : ConnectionPointType.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   /**
    * @param value
    */
   private ConnectionPointType(int value)
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
   public static ConnectionPointType getByValue(int value)
   {
      final ConnectionPointType element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(ConnectionPointType.class.getName(), "Unknown element " + value);
         return INDIRECT; // fallback
      }
      return element;
   }
}
