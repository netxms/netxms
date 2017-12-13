/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
 * Rack attribute
 */
public enum RackElementType
{
   PATCH_PANEL(0),
   FILLER_PANEL(1),
   ORGANISER(2);
   
   private int value;
   private static Map<Integer, RackElementType> lookupTable = new HashMap<Integer, RackElementType>();
   
   static
   {
      for(RackElementType element : RackElementType.values())
      {
         lookupTable.put(element.value, element);
      }
   }
   
   /**
    * Internal constructor
    * 
    * @param value integer value
    */
   private RackElementType(int value)
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
    * Get enum by integer value
    * 
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static RackElementType getByValue(int value)
   {
      final RackElementType element = lookupTable.get(value);
      if (element == null)
      {
         Logger.warning(RackElementType.class.getName(), "Unknown element " + value);
         return FILLER_PANEL; // fall-back
      }
      return element;
   }
}
