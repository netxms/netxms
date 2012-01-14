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
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.datacollection.widgets.DciList;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for DCI list
 */
public class DciListComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		
		DciValue v1 = (DciValue)e1;
		DciValue v2 = (DciValue)e2;

		switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"))
		{
			case DciList.COLUMN_ID:
				result = (int)(v1.getId() - v2.getId());
				break;
			case DciList.COLUMN_PARAMETER:
				result = v1.getName().compareToIgnoreCase(v2.getName());
				break;
			case DciList.COLUMN_DESCRIPTION:
				result = v1.getDescription().compareToIgnoreCase(v2.getDescription());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
