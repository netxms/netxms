/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
 * Radio band
 */
public enum RadioBand
{
   UNKNOWN(0, "Unknown"),
   WIFI_2_4_GHZ(1, "2.4 GHz"),
   WIFI_3_65_GHZ(2, "3.65 GHz"),
   WIFI_5_GHZ(3, "5 GHz"),
   WIFI_6_GHZ(4, "6 GHz");

   private static Logger logger = LoggerFactory.getLogger(RadioBand.class);
   private static Map<Integer, RadioBand> lookupTable = new HashMap<Integer, RadioBand>();
   static
   {
      for(RadioBand element : RadioBand.values())
      {
         lookupTable.put(element.value, element);
      }
   }

   private int value;
   private String displayName;

   /**
    * Internal constructor
    * 
    * @param value integer value
    * @param displayName display name
    */
   private RadioBand(int value, String displayName)
   {
      this.value = value;
      this.displayName = displayName;
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
    * @see java.lang.Enum#toString()
    */
   @Override
   public String toString()
   {
      return displayName;
   }

   /**
    * Get enum element by integer value
    * 
    * @param value integer value
    * @return enum element corresponding to given integer value or fall-back element for invalid value
    */
   public static RadioBand getByValue(int value)
   {
      final RadioBand element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return UNKNOWN; // fall-back
      }
      return element;
   }
}
