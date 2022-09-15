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
 * OSPF interface type
 */
public enum OSPFInterfaceType
{
   UNKNOWN(0, ""),
   BROADCAST(1, "BROADCAST"),
   NBMA(2, "NBMA"),
   POINT_TO_POINT(3, "PT-TO-PT"),
   POINT_TO_MULTIPOINT(4, "PT-TO-MP");

   private static Logger logger = LoggerFactory.getLogger(OSPFInterfaceType.class);
   private static Map<Integer, OSPFInterfaceType> lookupTable = new HashMap<Integer, OSPFInterfaceType>();
   static
   {
      for(OSPFInterfaceType element : OSPFInterfaceType.values())
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
   private OSPFInterfaceType(int value, String text)
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
   public static OSPFInterfaceType getByValue(int value)
   {
      final OSPFInterfaceType element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return UNKNOWN; // fall-back
      }
      return element;
   }
}
