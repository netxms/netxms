/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.topology.views.helpers;

import java.net.InetAddress;
import java.net.UnknownHostException;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.topology.views.HostSearchResults;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for ConnectionPoint objects 
 *
 */
public class ConnectionPointComparator extends ViewerComparator
{
	private static final long serialVersionUID = 1L;

	private ITableLabelProvider labelProvider;
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	
	/**
	 * @param labelProvider
	 */
	public ConnectionPointComparator(ITableLabelProvider labelProvider)
	{
		super();
		this.labelProvider = labelProvider;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");
		switch(column)
		{
			case HostSearchResults.COLUMN_SEQUENCE:
				result = (Integer)((ConnectionPoint)e1).getData() - (Integer)((ConnectionPoint)e2).getData();
				break;
			case HostSearchResults.COLUMN_NODE:
			case HostSearchResults.COLUMN_INTERFACE:
			case HostSearchResults.COLUMN_MAC_ADDRESS:
			case HostSearchResults.COLUMN_SWITCH:
			case HostSearchResults.COLUMN_PORT:
				result = labelProvider.getColumnText(e1, column).compareToIgnoreCase(labelProvider.getColumnText(e2, column));
				break;
			case HostSearchResults.COLUMN_IP_ADDRESS:
				Interface iface1 = (Interface)session.findObjectById(((ConnectionPoint)e1).getLocalInterfaceId(), Interface.class);
				Interface iface2 = (Interface)session.findObjectById(((ConnectionPoint)e2).getLocalInterfaceId(), Interface.class);
				try
				{
					InetAddress a1 = ((ConnectionPoint)e1).getLocalIpAddress();
					InetAddress a2 = ((ConnectionPoint)e2).getLocalIpAddress();
					if (a1 == null)
						a1 = (iface1 != null) ? iface1.getPrimaryIP() : InetAddress.getByAddress(new byte[] { 0, 0, 0, 0});
					if (a2 == null)
						a2 = (iface2 != null) ? iface2.getPrimaryIP() : InetAddress.getByAddress(new byte[] { 0, 0, 0, 0});
					result = ComparatorHelper.compareInetAddresses(a1, a2);
				}
				catch(UnknownHostException e)
				{
					result = 0;
				}
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
