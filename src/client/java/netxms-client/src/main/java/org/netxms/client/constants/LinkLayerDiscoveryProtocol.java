/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
 * Link layer discovery protocols
 */
public enum LinkLayerDiscoveryProtocol
{
   UNKNOWN(0),
   FDB(1),
   CDP(2),
   LLDP(3),
   NDP(4),
   EDP(5),
   STP(6),
   OTHER(7),
   MANUAL(8);

   private static Logger logger = LoggerFactory.getLogger(LinkLayerDiscoveryProtocol.class);
   private static Map<Integer, LinkLayerDiscoveryProtocol> lookupTable = new HashMap<Integer, LinkLayerDiscoveryProtocol>();
   static
   {
      for(LinkLayerDiscoveryProtocol element : LinkLayerDiscoveryProtocol.values())
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
   private LinkLayerDiscoveryProtocol(int value)
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
   public static LinkLayerDiscoveryProtocol getByValue(int value)
   {
      final LinkLayerDiscoveryProtocol element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return UNKNOWN; // fallback
      }
      return element;
   }
}
