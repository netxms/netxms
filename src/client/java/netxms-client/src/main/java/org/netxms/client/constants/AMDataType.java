/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Reden Solutions
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
 * Asset management attribute data type 
 */
public enum AMDataType
{
   STRING(0),
   INTEGER(1),
   NUMBER(2),
   BOOLEAN(3),
   ENUM(4),
   MAC_ADDRESS(5),
   IP_ADDRESS(6),
   UUID(7),
   OBJECT_REFERENCE(8);
   
   private static Logger logger = LoggerFactory.getLogger(AMDataType.class);
   private static Map<Integer, AMDataType> lookupTable = new HashMap<Integer, AMDataType>();
   static 
   {
      for(AMDataType element : AMDataType.values())
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
   private AMDataType(int value)
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
   public static AMDataType getByValue(int value)
   {
      final AMDataType element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return STRING; //fall-back 
      }
      return element;
   }   
}
