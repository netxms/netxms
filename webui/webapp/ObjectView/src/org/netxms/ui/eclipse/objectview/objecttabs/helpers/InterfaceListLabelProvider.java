/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.objectview.objecttabs.InterfacesTab;

/**
 * Label provider for interface list
 */
public class InterfaceListLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
	private static final long serialVersionUID = 1L;
	private static final String[] ifaceExpectedState = { "UP", "DOWN", "IGNORE" };
	
	private Node node = null;
	
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
				return iface.getPrimaryIP().isAnyLocalAddress() ? null : iface.getPrimaryIP().getHostAddress();
		}
		return null;
	}

	/**
	 * @param node the node to set
	 */
	public void setNode(Node node)
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
