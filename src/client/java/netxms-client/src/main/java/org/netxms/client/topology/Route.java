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
package org.netxms.client.topology;

import java.net.InetAddress;
import org.netxms.base.InetAddressEx;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.RoutingProtocol;

/**
 * IP route
 */
public class Route
{
   private InetAddressEx destination;
   private InetAddress nextHop;
   private int ifIndex;
   private String ifName;
   private int type;
   private int metric;
   private RoutingProtocol protocol;

   /**
    * Create route object from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId object base id
    */
   public Route(NXCPMessage msg, long baseId)
   {
      destination = msg.getFieldAsInetAddressEx(baseId);
      nextHop = msg.getFieldAsInetAddress(baseId + 1);
      ifIndex = msg.getFieldAsInt32(baseId + 2);
      type = msg.getFieldAsInt32(baseId + 3);
      metric = msg.getFieldAsInt32(baseId + 4);
      protocol = RoutingProtocol.getByValue(msg.getFieldAsInt32(baseId + 5));
      ifName = msg.getFieldAsString(baseId + 6);
   }

   /**
    * @return the destination
    */
   public InetAddressEx getDestination()
   {
      return destination;
   }

   /**
    * @return the nextHop
    */
   public InetAddress getNextHop()
   {
      return nextHop;
   }

   /**
    * @return the ifIndex
    */
   public int getIfIndex()
   {
      return ifIndex;
   }

   /**
    * Get interface name.
    *
    * @return interface name
    */
   public String getIfName()
   {
      return ifName;
   }

   /**
    * @return the type
    */
   public int getType()
   {
      return type;
   }

   /**
    * @return the metric
    */
   public int getMetric()
   {
      return metric;
   }

   /**
    * @return the protocol
    */
   public RoutingProtocol getProtocol()
   {
      return protocol;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "Route [" + destination.toString() + " gw=" + nextHop.getHostAddress() + " iface=" + ifIndex + " type=" + type + " metric=" + metric + " proto=" + protocol + "]";
   }
}
