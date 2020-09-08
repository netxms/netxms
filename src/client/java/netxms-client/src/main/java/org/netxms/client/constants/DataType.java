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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Data types for DCI
 */
public enum DataType
{
   INT32(0),
   UINT32(1),
   INT64(2),
   UINT64(3),
   STRING(4),
   FLOAT(5),
   NULL(6),
   COUNTER32(7),
   COUNTER64(8);

   private static Logger logger = LoggerFactory.getLogger(DataType.class);
   private static Map<Integer, DataType> lookupTable = new HashMap<Integer, DataType>();
   static
   {
      for(DataType element : DataType.values())
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
   private DataType(int value)
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
    * Returns true if this type is signed
    * 
    * @return true if this type is signed
    */
   public boolean isSigned()
   {
      return (this == INT32) || (this == INT64) || (this == FLOAT);
   }

   /**
    * Returns true if this type is 64 bit
    * 
    * @return true if this type is 64 bit
    */
   public boolean is64bit()
   {
      return (this == INT64) || (this == UINT64) || (this == COUNTER64);
   }
   
   /**
    * Get enum element by integer value
    * 
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static DataType getByValue(int value)
   {
      final DataType element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return INT32; // fallback
      }
      return element;
   }
   
   /**
    * Get type suitable for comparing two values of given types
    * 
    * @param t1 Type 1
    * @param t2 Type 2
    * @return resulting type
    */
   public static DataType getTypeForCompare(DataType t1, DataType t2)
   {
      if (t1 == t2)
         return t1;
      if ((t1 == STRING) || (t2 == STRING))
         return STRING;
      if ((t1 == FLOAT) || (t2 == FLOAT))
         return FLOAT;
      if (t1.is64bit() || t2.is64bit())
         return t1.isSigned() && t2.isSigned() ? INT64 : UINT64;
      return t1.isSigned() && t2.isSigned() ? INT32 : UINT32;
   }
}
