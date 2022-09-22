/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.client.constants.OSPFNeighborState;

/**
 * OSPF neighbor information
 */
public class OSPFNeighbor
{
   private InetAddress routerId;
   private InetAddress ipAddress;
   private long nodeId;
   private boolean virtual;
   private int interfaceIndex;
   private long interfaceId;
   private InetAddress areaId;
   private OSPFNeighborState state;

   /**
    * Create neighbor object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public OSPFNeighbor(NXCPMessage msg, long baseId)
   {
      routerId = msg.getFieldAsInetAddress(baseId);
      nodeId = msg.getFieldAsInt64(baseId + 1);
      ipAddress = msg.getFieldAsInetAddress(baseId + 2);
      interfaceIndex = msg.getFieldAsInt32(baseId + 3);
      interfaceId = msg.getFieldAsInt64(baseId + 4);
      areaId = msg.getFieldAsInetAddress(baseId + 5);
      virtual = msg.getFieldAsBoolean(baseId + 6);
      state = OSPFNeighborState.getByValue(msg.getFieldAsInt32(baseId + 7));
   }

   /**
    * Get OSPF router ID.
    *
    * @return OSPF router ID
    */
   public InetAddress getRouterId()
   {
      return routerId;
   }

   /**
    * Get IP address of this OSPF neighbor.
    *
    * @return IP address of this OSPF neighbor
    */
   public InetAddress getIpAddress()
   {
      return ipAddress;
   }

   /**
    * Get node ID of this OSPF neighbor. Will return 0 if node is not known.
    *
    * @return node ID of this OSPF neighbor or 0
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * Check if this neighbor is a virtual one.
    *
    * @return true if this neighbor is virtual
    */
   public boolean isVirtual()
   {
      return virtual;
   }

   /**
    * Get interface index of local interface where this neighbor is connected. Will return 0 for virtual neighbors.
    *
    * @return interface index of local interface where this neighbor is connected or 0
    */
   public int getInterfaceIndex()
   {
      return interfaceIndex;
   }

   /**
    * Get ID of local interface object where this neighbor is connected. Will return 0 for virtual neighbors.
    *
    * @return ID of local interface object where this neighbor is connected or 0
    */
   public long getInterfaceId()
   {
      return interfaceId;
   }

   /**
    * Get transit area ID for virtual neighbor. Will return null for non-virtual neighbors.
    *
    * @return area ID for virtual neighbor
    */
   public InetAddress getAreaId()
   {
      return virtual ? areaId : null;
   }

   /**
    * Get neighbor state.
    *
    * @return neighbor state
    */
   public OSPFNeighborState getState()
   {
      return state;
   }
}
