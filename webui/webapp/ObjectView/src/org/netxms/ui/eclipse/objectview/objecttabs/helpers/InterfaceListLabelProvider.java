/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.InterfacesTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for interface list
 */
public class InterfaceListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
	private static final String[] ifaceExpectedState = { Messages.get().InterfaceListLabelProvider_StateUp, Messages.get().InterfaceListLabelProvider_StateDown, Messages.get().InterfaceListLabelProvider_StateIgnore };
	
	private AbstractNode node = null;
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		if (node == null)
			return null;
		
		Interface iface = (Interface)element;
		switch(columnIndex)
		{
			case InterfacesTab.COLUMN_8021X_BACKEND_STATE:
				if (node.is8021xSupported() && iface.isPhysicalPort())
					return iface.getDot1xBackendStateAsText();
				return null;
			case InterfacesTab.COLUMN_8021X_PAE_STATE:
				if (node.is8021xSupported() && iface.isPhysicalPort())
					return iface.getDot1xPaeStateAsText();
				return null;
			case InterfacesTab.COLUMN_ADMIN_STATE:
				return iface.getAdminStateAsText();
         case InterfacesTab.COLUMN_ALIAS:
            return iface.getAlias();
			case InterfacesTab.COLUMN_DESCRIPTION:
				return iface.getDescription();
			case InterfacesTab.COLUMN_EXPECTED_STATE:
				try
				{
					return ifaceExpectedState[iface.getExpectedState()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					return null;
				}
			case InterfacesTab.COLUMN_ID:
				return Long.toString(iface.getObjectId());
			case InterfacesTab.COLUMN_INDEX:
				return Integer.toString(iface.getIfIndex());
         case InterfacesTab.COLUMN_MTU:
            return Integer.toString(iface.getMtu());
			case InterfacesTab.COLUMN_NAME:
				return iface.getObjectName();
			case InterfacesTab.COLUMN_OPER_STATE:
				return iface.getOperStateAsText();
			case InterfacesTab.COLUMN_PORT:
				if (iface.isPhysicalPort())
					return Integer.toString(iface.getPort());
				return null;
			case InterfacesTab.COLUMN_SLOT:
				if (iface.isPhysicalPort())
					return Integer.toString(iface.getSlot());
				return null;
			case InterfacesTab.COLUMN_STATUS:
				return StatusDisplayInfo.getStatusText(iface.getStatus());
			case InterfacesTab.COLUMN_TYPE:
				return Integer.toString(iface.getIfType());
			case InterfacesTab.COLUMN_MAC_ADDRESS:
				return iface.getMacAddress().toString();
			case InterfacesTab.COLUMN_IP_ADDRESS:
				return iface.getPrimaryIP().isAnyLocalAddress() ? null : iface.getPrimaryIP().getHostAddress() + "/" + iface.getSubnetMaskBits(); //$NON-NLS-1$
			case InterfacesTab.COLUMN_PEER_NAME:
				return getPeerName(iface);
			case InterfacesTab.COLUMN_PEER_MAC_ADDRESS:
				return getPeerMacAddress(iface);
			case InterfacesTab.COLUMN_PEER_IP_ADDRESS:
				return getPeerIpAddress(iface);
         case InterfacesTab.COLUMN_PEER_PROTOCOL:
            return getPeerProtocol(iface);
		}
		return null;
	}

	/**
	 * @param iface
	 * @return
	 */
	private String getPeerIpAddress(Interface iface)
	{
	   AbstractNode peer = (AbstractNode)session.findObjectById(iface.getPeerNodeId(), AbstractNode.class);
		if (peer == null)
			return null;
		if (peer.getPrimaryIP().isAnyLocalAddress())
			return null;
		return peer.getPrimaryIP().getHostAddress();
	}

	/**
	 * @param iface
	 * @return
	 */
	private String getPeerMacAddress(Interface iface)
	{
		Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
		return (peer != null) ? peer.getMacAddress().toString() : null;
	}

   /**
    * @param iface
    * @return
    */
   private String getPeerProtocol(Interface iface)
   {
      Interface peer = (Interface)session.findObjectById(iface.getPeerInterfaceId(), Interface.class);
      return (peer != null) ? iface.getPeerDiscoveryProtocol().toString() : null;
   }

	/**
	 * @param iface
	 * @return
	 */
	private String getPeerName(Interface iface)
	{
	   AbstractNode peer = (AbstractNode)session.findObjectById(iface.getPeerNodeId(), AbstractNode.class);
		return (peer != null) ? peer.getObjectName() : null;
	}

	/**
	 * @param node the node to set
	 */
	public void setNode(AbstractNode node)
	{
		this.node = node;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
	 */
	@Override
	public Color getForeground(Object element, int columnIndex)
	{
		Interface iface = (Interface)element;
		switch(columnIndex)
		{
			case InterfacesTab.COLUMN_STATUS:
				return StatusDisplayInfo.getStatusColor(iface.getStatus());
			case InterfacesTab.COLUMN_OPER_STATE:
				switch(iface.getOperState())
				{
					case Interface.OPER_STATE_UP:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL);
					case Interface.OPER_STATE_DOWN:
						return StatusDisplayInfo.getStatusColor((iface.getAdminState() == Interface.ADMIN_STATE_DOWN) ? ObjectStatus.DISABLED : ObjectStatus.CRITICAL);
					case Interface.OPER_STATE_TESTING:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.TESTING);
				}
				return StatusDisplayInfo.getStatusColor(ObjectStatus.UNKNOWN);
			case InterfacesTab.COLUMN_ADMIN_STATE:
				switch(iface.getAdminState())
				{
					case Interface.ADMIN_STATE_UP:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.NORMAL);
					case Interface.ADMIN_STATE_DOWN:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.DISABLED);
					case Interface.ADMIN_STATE_TESTING:
						return StatusDisplayInfo.getStatusColor(ObjectStatus.TESTING);
				}
				return StatusDisplayInfo.getStatusColor(ObjectStatus.UNKNOWN);
			default:
				return null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
	 */
	@Override
	public Color getBackground(Object element, int columnIndex)
	{
		return null;
	}
}
