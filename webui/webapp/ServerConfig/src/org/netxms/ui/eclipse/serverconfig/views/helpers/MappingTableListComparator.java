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
package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.api.client.mt.MappingTableDescriptor;
import org.netxms.ui.eclipse.serverconfig.views.MappingTables;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for mapping tables list
 */
public class MappingTableListComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		
		MappingTableDescriptor d1 = (MappingTableDescriptor)e1;
		MappingTableDescriptor d2 = (MappingTableDescriptor)e2;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case MappingTables.COLUMN_ID:
				result = d1.getId() - d2.getId();
				break;
			case MappingTables.COLUMN_NAME:
				result = d1.getName().compareToIgnoreCase(d2.getName());
				break;
			case MappingTables.COLUMN_DESCRIPTION:
				result = d1.getDescription().compareToIgnoreCase(d2.getDescription());
				break;
			case MappingTables.COLUMN_FLAGS:
				result = d1.getFlags() - d2.getFlags();
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
