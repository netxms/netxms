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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.InetAddressEx;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.interfaces.NodeComponent;

/**
 * VPN connector object
 */
public class VPNConnector extends GenericObject implements NodeComponent
{
   private long peerGatewayId;
   private List<InetAddressEx> localSubnets;
   private List<InetAddressEx> remoteSubnets;

   /**
    * Create from NXCP message
    * 
    * @param msg NXCP message
    * @param session owning client session
    */
   public VPNConnector(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      
      peerGatewayId = msg.getFieldAsInt64(NXCPCodes.VID_PEER_GATEWAY);
      
      long fieldId = NXCPCodes.VID_VPN_NETWORK_BASE;
      int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_LOCAL_NETS);
      localSubnets = new ArrayList<InetAddressEx>(count);
      for(int i = 0; i < count; i++)
      {
         localSubnets.add(msg.getFieldAsInetAddressEx(fieldId++));
      }

      count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_REMOTE_NETS);
      remoteSubnets = new ArrayList<InetAddressEx>(count);
      for(int i = 0; i < count; i++)
      {
         remoteSubnets.add(msg.getFieldAsInetAddressEx(fieldId++));
      }
   }

   /**
    * @see org.netxms.client.objects.interfaces.NodeComponent#getParentNode()
    */
   @Override
   public AbstractNode getParentNode()
   {
      AbstractNode node = null;
      synchronized(parents)
      {
         for(Long id : parents)
         {
            AbstractObject object = session.findObjectById(id);
            if (object instanceof AbstractNode)
            {
               node = (AbstractNode)object;
               break;
            }
         }
      }
      return node;
   }
   
   /**
    * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "VPNConnector";
   }

   /**
    * @return the peerGatewayId
    */
   public long getPeerGatewayId()
   {
      return peerGatewayId;
   }

   /**
    * @return the localSubnets
    */
   public List<InetAddressEx> getLocalSubnets()
   {
      return localSubnets;
   }

   /**
    * @return the remoteSubnets
    */
   public List<InetAddressEx> getRemoteSubnets()
   {
      return remoteSubnets;
   }
}
