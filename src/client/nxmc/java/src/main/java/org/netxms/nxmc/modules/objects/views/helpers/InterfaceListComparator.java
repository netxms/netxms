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
package org.netxms.nxmc.modules.objects.views.helpers;

import java.net.InetAddress;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.InterfacesView;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for interface list
 */
public class InterfaceListComparator extends ViewerComparator
{
   private static final MacAddress ZERO_MAC_ADDRESS = new MacAddress();

   private NXCSession session = Registry.getSession();

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final Interface iface1 = (Interface)e1;
		final Interface iface2 = (Interface)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");

		int result;
		switch(column)
		{
         case InterfacesView.COLUMN_NODE:
            result = iface1.getParentNode().getObjectName().compareTo(iface2.getParentNode().getObjectName());
            break;
         case InterfacesView.COLUMN_8021X_BACKEND_STATE:
				result = iface1.getDot1xBackendState() - iface2.getDot1xBackendState();
				break;
         case InterfacesView.COLUMN_8021X_PAE_STATE:
				result = iface1.getDot1xPaeState() - iface2.getDot1xPaeState();
				break;
         case InterfacesView.COLUMN_ADMIN_STATE:
				result = iface1.getAdminState() - iface2.getAdminState();
				break;
         case InterfacesView.COLUMN_ALIAS:
            result = ComparatorHelper.compareStringsNatural(iface1.getAlias(), iface2.getAlias());
            break;
         case InterfacesView.COLUMN_DESCRIPTION:
            result = ComparatorHelper.compareStringsNatural(iface1.getDescription(), iface2.getDescription());
				break;
         case InterfacesView.COLUMN_EXPECTED_STATE:
				result = iface1.getExpectedState() - iface2.getExpectedState();
				break;
         case InterfacesView.COLUMN_ID:
				result = Long.signum(iface1.getObjectId() - iface2.getObjectId());
				break;
         case InterfacesView.COLUMN_INDEX:
				result = iface1.getIfIndex() - iface2.getIfIndex();
				break;
         case InterfacesView.COLUMN_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(iface1.getFirstUnicastAddress(), iface2.getFirstUnicastAddress());
            break;
         case InterfacesView.COLUMN_MAC_ADDRESS:
            result = iface1.getMacAddress().compareTo(iface2.getMacAddress());
            break;
         case InterfacesView.COLUMN_MAX_SPEED:
            result = Long.signum(iface1.getMaxSpeed() - iface2.getMaxSpeed());
            break;
         case InterfacesView.COLUMN_MTU:
            result = iface1.getMtu() - iface2.getMtu();
            break;
         case InterfacesView.COLUMN_NAME:
            result = ComparatorHelper.compareStringsNatural(iface1.getObjectName(), iface2.getObjectName());
				break;
         case InterfacesView.COLUMN_NIC_VENDOR:
            result = getVendor(iface1).compareTo(getVendor(iface2));
            break;
         case InterfacesView.COLUMN_OPER_STATE:
				result = iface1.getOperState() - iface2.getOperState();
				break;
         case InterfacesView.COLUMN_OSPF_AREA:
            result = Boolean.compare(iface1.isOSPF(), iface2.isOSPF());
            if ((result == 0) && iface1.isOSPF())
               result = ComparatorHelper.compareInetAddresses(iface1.getOSPFArea(), iface2.getOSPFArea());
            break;
         case InterfacesView.COLUMN_OSPF_STATE:
            result = Boolean.compare(iface1.isOSPF(), iface2.isOSPF());
            if ((result == 0) && iface1.isOSPF())
               result = iface1.getOSPFState().getText().compareTo(iface2.getOSPFState().getText());
            break;
         case InterfacesView.COLUMN_OSPF_TYPE:
            result = Boolean.compare(iface1.isOSPF(), iface2.isOSPF());
            if ((result == 0) && iface1.isOSPF())
               result = iface1.getOSPFType().getText().compareTo(iface2.getOSPFType().getText());
            break;
         case InterfacesView.COLUMN_PEER_INTERFACE:
            result = ComparatorHelper.compareStringsNatural(getPeerInterfaceName(iface1), getPeerInterfaceName(iface2));
            break;
         case InterfacesView.COLUMN_PEER_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(getPeerIpAddress(iface1), getPeerIpAddress(iface2));
            break;
         case InterfacesView.COLUMN_PEER_LAST_UPDATED:
            result = Long.compare(iface1.getPeerLastUpdateTime().getTime(), iface2.getPeerLastUpdateTime().getTime());
            break;
         case InterfacesView.COLUMN_PEER_MAC_ADDRESS:
            result = getPeerMacAddress(iface1).compareTo(getPeerMacAddress(iface2));
            break;
         case InterfacesView.COLUMN_PEER_NIC_VENDOR:
            result = getPeerVendor(iface1).compareTo(getPeerVendor(iface2));
            break;
         case InterfacesView.COLUMN_PEER_NODE:
            result = ComparatorHelper.compareStringsNatural(getPeerNodeName(iface1), getPeerNodeName(iface2));
            break;
         case InterfacesView.COLUMN_PEER_PROTOCOL:
            result = getPeerProtocol(iface1).compareTo(getPeerProtocol(iface2));
            break;
			case InterfacesView.COLUMN_PHYSICAL_LOCATION:
			   if (iface1.isPhysicalPort() && iface2.isPhysicalPort())
			   {
	            result = iface1.getChassis() - iface2.getChassis();
	            if (result == 0)
	               result = iface1.getModule() - iface2.getModule();
	            if (result == 0)
	               result = iface1.getPIC() - iface2.getPIC();
	            if (result == 0)
	               result = iface1.getPort() - iface2.getPort();
			   }
			   else if (iface1.isPhysicalPort())
			   {
			      result = 1;
			   }
            else if (iface2.isPhysicalPort())
            {
               result = -1;
            }
            else
            {
               result = 0;
            }
				break;
         case InterfacesView.COLUMN_SPEED:
            result = Long.signum(iface1.getSpeed() - iface2.getSpeed());
            break;
         case InterfacesView.COLUMN_STATUS:
				result = iface1.getStatus().compareTo(iface2.getStatus());
				break;
         case InterfacesView.COLUMN_STP_STATE:
            result = iface1.getStpPortState().getText().compareTo(iface2.getStpPortState().getText());
            break;
         case InterfacesView.COLUMN_TYPE:
				result = iface1.getIfType() - iface2.getIfType();
				break;
         case InterfacesView.COLUMN_UTILIZATION:
            result = (iface1.getInboundUtilization() + iface1.getOutboundUtilization()) - (iface2.getInboundUtilization() + iface2.getOutboundUtilization());
            break;
			default:
				result = 0;
				break;
		}

		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}

   /**
    * @param iface
    * @return
    */
   private InetAddress getPeerIpAddress(Interface iface)
   {
      AbstractNode peer = session.findObjectById(iface.getPeerNodeId(), AbstractNode.class);
      if (peer == null)
         return null;
      if (!peer.getPrimaryIP().isValidUnicastAddress())
         return null;
      return peer.getPrimaryIP().getAddress();
   }

   /**
    * @param iface
    * @return
    */
   private MacAddress getPeerMacAddress(Interface iface)
   {
      Interface peer = session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      return (peer != null) ? peer.getMacAddress() : ZERO_MAC_ADDRESS;
   }

   /**
    * @param iface
    * @return
    */
   private String getPeerVendor(Interface iface)
   {
      Interface peer = session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      if (peer == null)
         return "";
      String vendor = session.getVendorByMac(peer.getMacAddress(), null);
      return (vendor != null) ? vendor : "";
   }

   /**
    * @param iface
    * @return
    */
   private String getPeerProtocol(Interface iface)
   {
      Interface peer = session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      return (peer != null) ? iface.getPeerDiscoveryProtocol().toString() : "";
   }

   /**
    * @param iface
    * @return
    */
   private String getPeerNodeName(Interface iface)
   {
      AbstractNode peer = session.findObjectById(iface.getPeerNodeId(), AbstractNode.class);
      return (peer != null) ? peer.getObjectName() : "";
   }

   /**
    * @param iface
    * @return
    */
   private String getPeerInterfaceName(Interface iface)
   {
      Interface peer = session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      return (peer != null) ? peer.getObjectName() : "";
   }

   /**
    * Get NIC vendor
    *
    * @param iface interface
    * @return NIC vendor
    */
   private String getVendor(Interface iface)
   {
      String vendor = session.getVendorByMac(iface.getMacAddress(), null);
      return (vendor != null) ? vendor : "";
   }
}
