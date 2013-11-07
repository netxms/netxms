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
package org.netxms.ui.eclipse.switchmanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.ui.eclipse.switchmanager.views.Dot1xStatusView;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for 802.1x port state table
 */
public class Dot1xPortComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		Dot1xPortSummary p1 = (Dot1xPortSummary)e1;
		Dot1xPortSummary p2 = (Dot1xPortSummary)e2;
		
		int result;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case Dot1xStatusView.COLUMN_NODE:
				result = p1.getNodeName().compareToIgnoreCase(p2.getNodeName());
				if (result != 0)
					break;
				// No break intentionally - if node is the same, sort by port
			case Dot1xStatusView.COLUMN_PORT:
				result = Integer.signum(p1.getSlot() - p2.getSlot());
				if (result == 0)
					result = Integer.signum(p1.getPort() - p2.getPort());
				if (result != 0)
					break;
				// No break intentionally - if slot/port is the same
				// (usually because they are unknown), sort by interface name
			case Dot1xStatusView.COLUMN_INTERFACE:
				result = p1.getInterfaceName().compareToIgnoreCase(p2.getInterfaceName());
				break;
			case Dot1xStatusView.COLUMN_PAE_STATE:
				result = p1.getPaeStateAsText().compareTo(p2.getPaeStateAsText());
				break;
			case Dot1xStatusView.COLUMN_BACKEND_STATE:
				result = p1.getBackendStateAsText().compareTo(p2.getBackendStateAsText());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.DOWN) ? result : -result;
	}
}
