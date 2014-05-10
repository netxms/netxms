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
public enum AccessPointState
{
   ADOPTED(0),
   UNADOPTED(1);

   private int value;
   private static Map<Integer, AccessPointState> lookupTable = new HashMap<Integer, AccessPointState>();

   static
   {
      for(AccessPointState element : AccessPointState.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   /**
    * @param value
    */
   private AccessPointState(int value)
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
   public static AccessPointState getByValue(int value)
   {
      final AccessPointState element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(AccessPointState.class.getName(), "Unknown element " + value);
         return ADOPTED; // fallback
      }
      return element;
   }
}
