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
package org.netxms.nxmc.modules.tools.widgets.helpers;

import java.net.InetAddress;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.base.MacAddress;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.tools.widgets.SearchResult;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Comparator for ConnectionPoint objects 
 */
public class ConnectionPointComparator extends ViewerComparator
{   
	private ITableLabelProvider labelProvider;
   private NXCSession session = Registry.getSession();

	/**
	 * @param labelProvider
	 */
	public ConnectionPointComparator(ITableLabelProvider labelProvider)
	{
		super();
		this.labelProvider = labelProvider;
	}

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
		switch(column)
		{
			case SearchResult.COLUMN_SEQUENCE:
				result = (Integer)((ConnectionPoint)e1).getData() - (Integer)((ConnectionPoint)e2).getData();
				break;
			case SearchResult.COLUMN_INDEX:
            result = (Integer)((ConnectionPoint)e1).getInterfaceIndex() - (Integer)((ConnectionPoint)e2).getInterfaceIndex();
            break;			   
			case SearchResult.COLUMN_NODE:
			case SearchResult.COLUMN_INTERFACE:
         case SearchResult.COLUMN_NIC_VENDOR:
			case SearchResult.COLUMN_SWITCH:
			case SearchResult.COLUMN_PORT:
				result = labelProvider.getColumnText(e1, column).compareToIgnoreCase(labelProvider.getColumnText(e2, column));
				break;
         case SearchResult.COLUMN_MAC_ADDRESS:
            MacAddress m1 = ((ConnectionPoint)e1).getLocalMacAddress();
            MacAddress m2 = ((ConnectionPoint)e2).getLocalMacAddress();
            if (m1 == null)
               result = (m2 == null) ? 0 : -1;
            else
               result = m1.compareTo(m2);
            break;
			case SearchResult.COLUMN_IP_ADDRESS:
				Interface iface1 = (Interface)session.findObjectById(((ConnectionPoint)e1).getLocalInterfaceId(), Interface.class);
				Interface iface2 = (Interface)session.findObjectById(((ConnectionPoint)e2).getLocalInterfaceId(), Interface.class);
				InetAddress a1 = ((ConnectionPoint)e1).getLocalIpAddress();
				InetAddress a2 = ((ConnectionPoint)e2).getLocalIpAddress();
				if ((a1 == null) && (iface1 != null))
					a1 = iface1.getFirstUnicastAddress();
            if ((a2 == null) && (iface2 != null))
               a2 = iface2.getFirstUnicastAddress();
				result = ComparatorHelper.compareInetAddresses(a1, a2);
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
