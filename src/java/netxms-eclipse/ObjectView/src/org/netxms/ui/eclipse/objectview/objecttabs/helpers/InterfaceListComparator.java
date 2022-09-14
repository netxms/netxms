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
package org.netxms.ui.eclipse.objectview.objecttabs.helpers;

import java.net.InetAddress;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.objectview.objecttabs.InterfacesTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.tools.NaturalOrderComparator;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for interface list
 */
public class InterfaceListComparator extends ViewerComparator
{
   private static final MacAddress ZERO_MAC_ADDRESS = new MacAddress();

   private NXCSession session = ConsoleSharedData.getSession();

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final Interface iface1 = (Interface)e1;
		final Interface iface2 = (Interface)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$

		int result;
		switch(column)
		{
			case InterfacesTab.COLUMN_8021X_BACKEND_STATE:
				result = iface1.getDot1xBackendState() - iface2.getDot1xBackendState();
				break;
			case InterfacesTab.COLUMN_8021X_PAE_STATE:
				result = iface1.getDot1xPaeState() - iface2.getDot1xPaeState();
				break;
			case InterfacesTab.COLUMN_ADMIN_STATE:
				result = iface1.getAdminState() - iface2.getAdminState();
				break;
         case InterfacesTab.COLUMN_ALIAS:
            result = NaturalOrderComparator.compare(iface1.getAlias(), iface2.getAlias());
            break;
			case InterfacesTab.COLUMN_DESCRIPTION:
				result = NaturalOrderComparator.compare(iface1.getDescription(), iface2.getDescription());
				break;
			case InterfacesTab.COLUMN_EXPECTED_STATE:
				result = iface1.getExpectedState() - iface2.getExpectedState();
				break;
			case InterfacesTab.COLUMN_ID:
				result = Long.signum(iface1.getObjectId() - iface2.getObjectId());
				break;
			case InterfacesTab.COLUMN_INDEX:
				result = iface1.getIfIndex() - iface2.getIfIndex();
				break;
         case InterfacesTab.COLUMN_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(iface1.getFirstUnicastAddress(), iface2.getFirstUnicastAddress());
            break;
         case InterfacesTab.COLUMN_MAC_ADDRESS:
            result = iface1.getMacAddress().compareTo(iface2.getMacAddress());
            break;
         case InterfacesTab.COLUMN_MTU:
            result = iface1.getMtu() - iface2.getMtu();
            break;
         case InterfacesTab.COLUMN_NAME:
            result = NaturalOrderComparator.compare(iface1.getObjectName(), iface2.getObjectName());
				break;
         case InterfacesTab.COLUMN_OPER_STATE:
				result = iface1.getOperState() - iface2.getOperState();
				break;
         case InterfacesTab.COLUMN_OSPF_AREA:
            result = Boolean.compare(iface1.isOSPF(), iface2.isOSPF());
            if ((result == 0) && iface1.isOSPF())
               result = ComparatorHelper.compareInetAddresses(iface1.getOSPFArea(), iface2.getOSPFArea());
            break;
         case InterfacesTab.COLUMN_OSPF_STATE:
            result = Boolean.compare(iface1.isOSPF(), iface2.isOSPF());
            if ((result == 0) && iface1.isOSPF())
               result = iface1.getOSPFState().getText().compareTo(iface2.getOSPFState().getText());
            break;
         case InterfacesTab.COLUMN_OSPF_TYPE:
            result = Boolean.compare(iface1.isOSPF(), iface2.isOSPF());
            if ((result == 0) && iface1.isOSPF())
               result = iface1.getOSPFType().getText().compareTo(iface2.getOSPFType().getText());
            break;
         case InterfacesTab.COLUMN_PEER_INTERFACE:
            result = NaturalOrderComparator.compare(getPeerInterfaceName(iface1), getPeerInterfaceName(iface2));
            break;
         case InterfacesTab.COLUMN_PEER_IP_ADDRESS:
            result = ComparatorHelper.compareInetAddresses(getPeerIpAddress(iface1), getPeerIpAddress(iface2));
            break;
         case InterfacesTab.COLUMN_PEER_MAC_ADDRESS:
            result = getPeerMacAddress(iface1).compareTo(getPeerMacAddress(iface2));
            break;
         case InterfacesTab.COLUMN_PEER_NODE:
            result = NaturalOrderComparator.compare(getPeerNodeName(iface1), getPeerNodeName(iface2));
            break;
         case InterfacesTab.COLUMN_PEER_PROTOCOL:
            result = getPeerProtocol(iface1).compareTo(getPeerProtocol(iface2));
            break;
			case InterfacesTab.COLUMN_PHYSICAL_LOCATION:
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
         case InterfacesTab.COLUMN_SPEED:
            result = Long.signum(iface1.getSpeed() - iface2.getSpeed());
            break;
         case InterfacesTab.COLUMN_STATUS:
				result = iface1.getStatus().compareTo(iface2.getStatus());
				break;
         case InterfacesTab.COLUMN_TYPE:
				result = iface1.getIfType() - iface2.getIfType();
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
      AbstractNode peer = (AbstractNode)session.findObjectById(iface.getPeerNodeId(), AbstractNode.class);
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
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      return (peer != null) ? peer.getMacAddress() : ZERO_MAC_ADDRESS;
   }

   /**
    * @param iface
    * @return
    */
   private String getPeerProtocol(Interface iface)
   {
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      return (peer != null) ? iface.getPeerDiscoveryProtocol().toString() : "";
   }

   /**
    * @param iface
    * @return
    */
   private String getPeerNodeName(Interface iface)
   {
      AbstractNode peer = (AbstractNode)session.findObjectById(iface.getPeerNodeId(), AbstractNode.class);
      return (peer != null) ? peer.getObjectName() : "";
   }

   /**
    * @param iface
    * @return
    */
   private String getPeerInterfaceName(Interface iface)
   {
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      return (peer != null) ? peer.getObjectName() : "";
   }
}
