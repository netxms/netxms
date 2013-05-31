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
package org.netxms.ui.eclipse.objecttools.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objecttools.views.ObjectToolsEditor;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for ObjectTool objects
 *
 */
public class ObjectToolsComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		ObjectTool tool1 = (ObjectTool)e1;
		ObjectTool tool2 = (ObjectTool)e2;
		final int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$

		int result;
		switch(column)
		{
			case ObjectToolsEditor.COLUMN_ID:
				result = Long.signum(tool1.getId() - tool2.getId());
				break;
			case ObjectToolsEditor.COLUMN_NAME:
				result = tool1.getName().compareToIgnoreCase(tool2.getName());
				break;
			case ObjectToolsEditor.COLUMN_TYPE:
				result = ObjectToolsLabelProvider.getToolTypeName(tool1).compareTo(ObjectToolsLabelProvider.getToolTypeName(tool2));
				break;
			case ObjectToolsEditor.COLUMN_DESCRIPTION:
				result = tool1.getDescription().compareToIgnoreCase(tool2.getDescription());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
