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
 * Routing protocol
 */
public enum RoutingProtocol
{
   UNKNOWN(0),
   OTHER(1),
   LOCAL(2),
   NETMGMT(3),
   ICMP(4),
   EGP(5),
   GGP(6),
   HELLO(7),
   RIP(8),
   IS_IS(9),
   ES_IS(10),
   IGRP(11),
   BBN_SPF_IGP(12),
   OSPF(13),
   BGP(14),
   IDPR(15),
   EIGRP(16),
   DVMRP(17),
   RPL(18),
   DHCP(19);

   private static Logger logger = LoggerFactory.getLogger(RoutingProtocol.class);
   private static Map<Integer, RoutingProtocol> lookupTable = new HashMap<Integer, RoutingProtocol>();
   static
   {
      for(RoutingProtocol element : RoutingProtocol.values())
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
   private RoutingProtocol(int value)
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
   public static RoutingProtocol getByValue(int value)
   {
      final RoutingProtocol element = lookupTable.get(value);
      if (element == null)
      {
         logger.warn("Unknown element " + value);
         return UNKNOWN; // fall-back
      }
      return element;
   }
}
