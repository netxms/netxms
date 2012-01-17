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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.ui.eclipse.topology.views.HostSearchResults;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for ConnectionPoint objects 
 *
 */
public class ConnectionPointComparator extends ViewerComparator
{
	private ITableLabelProvider labelProvider;
	
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
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
