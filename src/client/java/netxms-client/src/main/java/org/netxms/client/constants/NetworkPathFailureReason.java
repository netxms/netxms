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
 * Network path failure reason
 */
public enum NetworkPathFailureReason
{
   NONE(0),
   ROUTER_DOWN(1),
   SWITCH_DOWN(2),
   WIRELESS_AP_DOWN(3),
   PROXY_NODE_DOWN(4),
   PROXY_AGENT_UNREACHABLE(5),
   VPN_TUNNEL_DOWN(6),
   ROUTING_LOOP(7),
   INTERFACE_DISABLED(8);

   private static Logger logger = LoggerFactory.getLogger(NetworkPathFailureReason.class);
   private static Map<Integer, NetworkPathFailureReason> lookupTable = new HashMap<Integer, NetworkPathFailureReason>();
   static
   {
      for(NetworkPathFailureReason element : NetworkPathFailureReason.values())
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
   private NetworkPathFailureReason(int value)
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
   public static NetworkPathFailureReason getByValue(int value)
   {
      final NetworkPathFailureReason element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return NONE; // fallback
      }
      return element;
   }
}
