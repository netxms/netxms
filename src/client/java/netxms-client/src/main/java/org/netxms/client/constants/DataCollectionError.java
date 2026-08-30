/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
 * Reason of the last data collection error
 */
public enum DataCollectionError
{
   SUCCESS(0),
   COMM_ERROR(1),
   NOT_SUPPORTED(2),
   IGNORE(3),
   NO_SUCH_INSTANCE(4),
   COLLECTION_ERROR(5),
   ACCESS_DENIED(6),
   INVALID_DATA(7);

   private static Logger logger = LoggerFactory.getLogger(DataCollectionError.class);
   private static Map<Integer, DataCollectionError> lookupTable = new HashMap<Integer, DataCollectionError>();
   static
   {
      for(DataCollectionError element : DataCollectionError.values())
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
   private DataCollectionError(int value)
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
   public static DataCollectionError getByValue(int value)
   {
      final DataCollectionError element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return COLLECTION_ERROR; // fallback
      }
      return element;
   }
}
