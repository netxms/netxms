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
 * OSPF interface state
 */
public enum OSPFInterfaceState
{
   UNKNOWN(0, ""),
   DOWN(1, "DOWN"),
   LOOPBACK(2, "LOOPBACK"),
   WAITING(3, "WAITING"),
   POINT_TO_POINT(4, "PT-TO-PT"),
   DESIGNATED_ROUTER(5, "DR"),
   BACKUP_DESIGNATED_ROUTER(6, "BDR"),
   OTHER_DESIGNATED_ROUTER(7, "ODR");

   private static Logger logger = LoggerFactory.getLogger(OSPFInterfaceState.class);
   private static Map<Integer, OSPFInterfaceState> lookupTable = new HashMap<Integer, OSPFInterfaceState>();
   static
   {
      for(OSPFInterfaceState element : OSPFInterfaceState.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;
   private String text;

   /**
    * Internal constructor
    * 
    * @param value integer value
    * @param text text value
    */
   private OSPFInterfaceState(int value, String text)
   {
      this.value = value;
      this.text = text;
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
    * Get text value
    *
    * @return text value
    */
   public String getText()
   {
      return text;
   }

   /**
    * Get enum element by integer value
    * 
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static OSPFInterfaceState getByValue(int value)
   {
      final OSPFInterfaceState element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return UNKNOWN; // fall-back
      }
      return element;
   }
}
