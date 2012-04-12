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
package org.netxms.ui.eclipse.objectmanager.propertypages.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.objects.ClusterSyncNetwork;
import org.netxms.ui.eclipse.objectmanager.propertypages.ClusterNetworks;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for cluster network list elements
 *
 */
public class NetworkListComparator extends ViewerComparator
{
	private static final long serialVersionUID = 1L;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");
		
		int result;
		switch(column)
		{
			case ClusterNetworks.COLUMN_ADDRESS:
				byte[] addr1 = ((ClusterSyncNetwork)e1).getSubnetAddress().getAddress();
				byte[] addr2 = ((ClusterSyncNetwork)e2).getSubnetAddress().getAddress();

				result = 0;
				for(int i = 0; (i < addr1.length) && (result == 0); i++)
				{
					result = Integer.signum(addr1[i] - addr2[i]);
				}
				break;
			case ClusterNetworks.COLUMN_NETMASK:
				byte[] mask1 = ((ClusterSyncNetwork)e1).getSubnetMask().getAddress();
				byte[] mask2 = ((ClusterSyncNetwork)e2).getSubnetMask().getAddress();

				result = 0;
				for(int i = 0; (i < mask1.length) && (result == 0); i++)
				{
					result = Integer.signum(mask1[i] - mask2[i]);
				}
				break;
			default:
				result = 0;
				break;
		}
		
		return (((SortableTableViewer) viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
