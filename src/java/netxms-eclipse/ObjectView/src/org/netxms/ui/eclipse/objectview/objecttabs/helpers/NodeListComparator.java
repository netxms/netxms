/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.objectview.objecttabs.NodesTab;
import org.netxms.ui.eclipse.tools.ComparatorHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for node list
 */
public class NodeListComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final AbstractNode node1 = (AbstractNode)e1;
		final AbstractNode node2 = (AbstractNode)e2;
		final int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
		
		int result;
		switch(column)
		{
			case NodesTab.COLUMN_SYS_DESCRIPTION:
				result = node1.getSystemDescription().compareToIgnoreCase(node2.getSystemDescription());
				break;
         case NodesTab.COLUMN_PLATFORM:
            result = node1.getPlatformName().compareToIgnoreCase(node2.getPlatformName());
            break;
         case NodesTab.COLUMN_AGENT_VERSION:
            result = node1.getAgentVersion().compareToIgnoreCase(node2.getAgentVersion());
            break;
			case NodesTab.COLUMN_ID:
				result = Long.signum(node1.getObjectId() - node2.getObjectId());
				break;
			case NodesTab.COLUMN_NAME:
				result = node1.getObjectName().compareToIgnoreCase(node2.getObjectName());
				break;
			case NodesTab.COLUMN_STATUS:
				result = node1.getStatus().compareTo(node2.getStatus());
				break;
			case NodesTab.COLUMN_IP_ADDRESS:
				result = ComparatorHelper.compareInetAddresses(node1.getPrimaryIP(), node2.getPrimaryIP());
				break;
			default:
				result = 0;
				break;
		}
		
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
