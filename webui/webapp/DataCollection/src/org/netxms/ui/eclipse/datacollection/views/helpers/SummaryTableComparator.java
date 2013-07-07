/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.ui.eclipse.datacollection.views.SummaryTableManager;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for DCI summary table descriptors
 */
public class SummaryTableComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		DciSummaryTableDescriptor d1 = (DciSummaryTableDescriptor)e1;
		DciSummaryTableDescriptor d2 = (DciSummaryTableDescriptor)e2;
		
		int result;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case SummaryTableManager.COLUMN_ID:
				result = d1.getId() - d2.getId();
				break;
			case SummaryTableManager.COLUMN_MENU_PATH:
				result = d1.getMenuPath().compareToIgnoreCase(d2.getMenuPath());
				break;
			case SummaryTableManager.COLUMN_TITLE:
				result = d1.getTitle().compareToIgnoreCase(d2.getTitle());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
