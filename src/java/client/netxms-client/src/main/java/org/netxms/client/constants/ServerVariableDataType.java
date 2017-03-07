/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
 * Data types for server configuration variables
 */
public enum ServerVariableDataType
{
   STRING(0),
   BOOLEAN(1),
   INTEGER(2),
   CHOICE(3),
   COLOR(4);

   private int value;
   private static Map<Integer, ServerVariableDataType> lookupTableValue = new HashMap<Integer, ServerVariableDataType>();
   private static Map<Integer, ServerVariableDataType> lookupTableCode = new HashMap<Integer, ServerVariableDataType>();

   static
   {
      for(ServerVariableDataType element : ServerVariableDataType.values())
      {
         lookupTableValue.put(element.value, element);
      }
      
      lookupTableCode.put((int)'S', STRING);
      lookupTableCode.put((int)'I', INTEGER);
      lookupTableCode.put((int)'B', BOOLEAN);
      lookupTableCode.put((int)'C', CHOICE);
      lookupTableCode.put((int)'H', COLOR);
   }

   /**
    * Internal constructor
    *  
    * @param value integer value
    */
   private ServerVariableDataType(int value)
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
   public static ServerVariableDataType getByValue(int value)
   {
      final ServerVariableDataType element = lookupTableValue.get(value);
      if (element == null)
      {
         Logger.warning(ServerVariableDataType.class.getName(), "Unknown element " + value);
         return STRING; // fallback
      }
      return element;
   }

   public static ServerVariableDataType getByCode(char code)
   {
      final ServerVariableDataType element = lookupTableCode.get((int)code);
      if (element == null)
      {
         Logger.warning(ServerVariableDataType.class.getName(), "Unknown type code " + code);
         return STRING; // fallback
      }
      return element;
   }
}
