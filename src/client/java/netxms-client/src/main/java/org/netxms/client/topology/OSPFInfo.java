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
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * OSPF information for node
 */
public class OSPFInfo
{
   private InetAddress routerId;
   private OSPFArea[] areas;
   private OSPFNeighbor[] neighbors;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    */
   public OSPFInfo(NXCPMessage msg)
   {
      routerId = msg.getFieldAsInetAddress(NXCPCodes.VID_OSPF_ROUTER_ID);

      int count = msg.getFieldAsInt32(NXCPCodes.VID_OSPF_AREA_COUNT);
      areas = new OSPFArea[count];
      long fieldId = NXCPCodes.VID_OSPF_AREA_LIST_BASE;
      for(int i = 0; i < count; i++, fieldId += 10)
         areas[i] = new OSPFArea(msg, fieldId);

      count = msg.getFieldAsInt32(NXCPCodes.VID_OSPF_NEIGHBOR_COUNT);
      neighbors = new OSPFNeighbor[count];
      fieldId = NXCPCodes.VID_OSPF_NEIGHBOR_LIST_BASE;
      for(int i = 0; i < count; i++, fieldId += 10)
         neighbors[i] = new OSPFNeighbor(msg, fieldId);
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
    * Get OSPF areas on this router.
    *
    * @return OSPF areas on this router
    */
   public OSPFArea[] getAreas()
   {
      return areas;
   }

   /**
    * Get OSFP neighbors of this router.
    *
    * @return OSFP neighbors of this router
    */
   public OSPFNeighbor[] getNeighbors()
   {
      return neighbors;
   }
}
