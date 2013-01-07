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
import org.netxms.api.client.mt.MappingTableEntry;
import org.netxms.ui.eclipse.serverconfig.views.MappingTableEditor;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for mapping table elements
 */
public class MappingTableEntryComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		
		MappingTableEntry me1 = (MappingTableEntry)e1;
		MappingTableEntry me2 = (MappingTableEntry)e2;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"))
		{
			case MappingTableEditor.COLUMN_KEY:
				result = me1.getKey().compareToIgnoreCase(me2.getKey());
				break;
			case MappingTableEditor.COLUMN_VALUE:
				result = me1.getValue().compareToIgnoreCase(me2.getValue());
				break;
			case MappingTableEditor.COLUMN_DESCRIPTION:
				result = me1.getDescription().compareToIgnoreCase(me2.getDescription());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
