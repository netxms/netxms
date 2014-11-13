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
 * Status codes
 */
public enum ObjectStatus
{
   NORMAL(0),
   WARNING(1),
   MINOR(2),
   MAJOR(3),
   CRITICAL(4),
   UNKNOWN(5),
   UNMANAGED(6),
   DISABLED(7),
   TESTING(8);

   private int value;
   private static Map<Integer, ObjectStatus> lookupTable = new HashMap<Integer, ObjectStatus>();

   static
   {
      for(ObjectStatus element : ObjectStatus.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   /**
    * @param value
    */
   private ObjectStatus(int value)
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
   public static ObjectStatus getByValue(int value)
   {
      final ObjectStatus element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(ObjectStatus.class.getName(), "Unknown element " + value);
         return UNKNOWN; // fallback
      }
      return element;
   }
}
