/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.ClusterResource;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.objectview.objecttabs.ClusterTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for cluster resource list
 */
public class ClusterResourceListComparator extends ViewerComparator
{
	private NXCSession session;
	
	/**
	 * 
	 */
	public ClusterResourceListComparator()
	{
		session = (NXCSession)ConsoleSharedData.getSession();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final ClusterResource r1 = (ClusterResource)e1;
		final ClusterResource r2 = (ClusterResource)e2;
		final int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID");
		
		int result;
		switch(column)
		{
			case ClusterTab.COLUMN_NAME:
				result = r1.getName().compareToIgnoreCase(r2.getName());
				break;
			case ClusterTab.COLUMN_VIP:
				byte[] addr1 = r1.getVirtualAddress().getAddress();
				byte[] addr2 = r2.getVirtualAddress().getAddress();

				result = 0;
				for(int i = 0; (i < addr1.length) && (result == 0); i++)
				{
					result = addr1[i] - addr2[i];
				}
				break;
			case ClusterTab.COLUMN_OWNER:
				result = getOwnerName(r1).compareToIgnoreCase(getOwnerName(r2));
				break;
			default:
				result = 0;
				break;
		}
		
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
	
	/**
	 * @param r
	 * @return
	 */
	private String getOwnerName(ClusterResource r)
	{
		long ownerId = r.getCurrentOwner();
		if (ownerId > 0)
		{
			Node owner = (Node)session.findObjectById(ownerId, Node.class);
			return (owner != null) ? owner.getObjectName() : "<" + Long.toString(ownerId) + ">";
		}
		else
		{
			return "NONE";
		}
	}
}
