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
package org.netxms.client.topology;

import java.net.InetAddress;
import org.netxms.base.NXCPMessage;

/**
 * IP route
 */
public class Route
{
   private InetAddress destination;
   private int prefixLength;
   private InetAddress nextHop;
   private int ifIndex;
   private int type;
   
   /**
    * Create route object from NXCP message
    * 
    * @param msg
    * @param baseId
    */
   public Route(NXCPMessage msg, long baseId)
   {
      destination = msg.getVariableAsInetAddress(baseId);
      prefixLength = msg.getVariableAsInteger(baseId + 1);
      nextHop = msg.getVariableAsInetAddress(baseId + 2);
      ifIndex = msg.getVariableAsInteger(baseId + 3);
      type = msg.getVariableAsInteger(baseId + 4);
   }

   /**
    * @return the destination
    */
   public InetAddress getDestination()
   {
      return destination;
   }

   /**
    * @return the prefixLength
    */
   public int getPrefixLength()
   {
      return prefixLength;
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
    * @return the type
    */
   public int getType()
   {
      return type;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "Route [" + destination.getHostAddress() + "/" + prefixLength + " gw=" + nextHop.getHostAddress() + " iface=" + ifIndex + " type=" + type + "]";
   }
}
