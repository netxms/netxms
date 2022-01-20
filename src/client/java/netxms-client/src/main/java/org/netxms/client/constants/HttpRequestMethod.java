/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
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
 * HTTP request methods
 */
public enum HttpRequestMethod
{
   GET(0),
   POST(1),
   PUT(2),
   DELETE(3),
   PATCH(4);

   private static Logger logger = LoggerFactory.getLogger(HttpRequestMethod.class);
   private static Map<Integer, HttpRequestMethod> lookupTable = new HashMap<Integer, HttpRequestMethod>();
   static
   {
      for(HttpRequestMethod element : HttpRequestMethod.values())
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
   private HttpRequestMethod(int value)
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
   public static HttpRequestMethod getByValue(int value)
   {
      final HttpRequestMethod element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return GET; // fallback
      }
      return element;
   }
}
