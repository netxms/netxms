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
package org.netxms.ui.eclipse.objectmanager.propertypages.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.base.InetAddressEx;
import org.netxms.ui.eclipse.objectmanager.propertypages.ClusterNetworks;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for cluster network list elements
 *
 */
public class NetworkListComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
		
		int result;
		switch(column)
		{
			case ClusterNetworks.COLUMN_ADDRESS:
				byte[] addr1 = ((InetAddressEx)e1).address.getAddress();
				byte[] addr2 = ((InetAddressEx)e2).address.getAddress();

				result = 0;
				for(int i = 0; (i < addr1.length) && (result == 0); i++)
				{
					result = Integer.signum(addr1[i] - addr2[i]);
				}
				break;
			case ClusterNetworks.COLUMN_NETMASK:
			   result = ((InetAddressEx)e1).mask - ((InetAddressEx)e2).mask;
				break;
			default:
				result = 0;
				break;
		}
		
		return (((SortableTableViewer) viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
