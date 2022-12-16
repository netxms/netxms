/**
 * NetXMS - open source network management system
 * Copyright (C) 2009 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views.helpers;

import java.util.List;
import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.base.InetAddressEx;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.VPNConnector;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.views.VpnView;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Label provider for VPNConnector
 */
public class VPNConnectorListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private NXCSession session = Registry.getSession();

   @Override
   public Color getForeground(Object element, int columnIndex)
   {
      VPNConnector vpn = (VPNConnector)element;
      switch(columnIndex)
      {
         case VpnView.COLUMN_STATUS:
            StatusDisplayInfo.getStatusColor(vpn.getStatus());
      }
      return null;
   }

   @Override
   public Color getBackground(Object element, int columnIndex)
   {
      return null;
   }

   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      VPNConnector vpn = (VPNConnector)element;
      switch(columnIndex)
      {
         case VpnView.COLUMN_ID:
            return Long.toString(vpn.getObjectId());
         case VpnView.COLUMN_NAME:
            return vpn.getObjectName();
         case VpnView.COLUMN_STATUS:
            return StatusDisplayInfo.getStatusText(vpn.getStatus());
         case VpnView.COLUMN_PEER_GATEWAY:
            return getPeerName(vpn);
         case VpnView.COLUMN_LOCAL_SUBNETS:
            return getSubnetsAsString(vpn.getLocalSubnets());
         case VpnView.COLUMN_REMOTE_SUBNETS:
            return getSubnetsAsString(vpn.getRemoteSubnets());
            
      }
      return null;
   }

   /**
    * @param iface
    * @return
    */
   public String getPeerName(VPNConnector vpn)
   {
      AbstractNode peer = (AbstractNode)session.findObjectById(vpn.getPeerGatewayId(), AbstractNode.class);
      return (peer != null) ? peer.getObjectName() : null;
   }

   /**
    * @param iface
    * @return
    */
   public String getSubnetsAsString(List<InetAddressEx> subnetList)
   {
      StringBuilder sb = new StringBuilder();
      for(int i = 0; i < subnetList.size(); i++)
      {
         sb.append(subnetList.get(i).toString());
         if(i+1 != subnetList.size())
            sb.append(", ");
      }
      return sb.toString();
   }
}
