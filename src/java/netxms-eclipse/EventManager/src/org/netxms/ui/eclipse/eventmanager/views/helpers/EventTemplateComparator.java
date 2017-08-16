/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.eventmanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.events.EventObject;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.eventmanager.views.EventConfigurator;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Event template comparator
 */
public class EventTemplateComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case EventConfigurator.COLUMN_CODE:
				result = (int)(((EventObject)e1).getCode() - ((EventObject)e2).getCode());
				break;
			case EventConfigurator.COLUMN_NAME:
				result = ((EventObject)e1).getName().compareToIgnoreCase(((EventObject)e2).getName());
				break;
			case EventConfigurator.COLUMN_SEVERITY:
			   if (e1 instanceof EventTemplate && e2 instanceof EventTemplate)
			      result = ((EventTemplate)e1).getSeverity().compareTo(((EventTemplate)e2).getSeverity());
			   else
			      result = 0;
				break;
			case EventConfigurator.COLUMN_FLAGS:
            if (e1 instanceof EventTemplate && e2 instanceof EventTemplate)
               result = ((EventTemplate)e1).getFlags() - ((EventTemplate)e2).getFlags();
            else
               result = 0;
				break;
			case EventConfigurator.COLUMN_MESSAGE:
            if (e1 instanceof EventTemplate && e2 instanceof EventTemplate)
               result = ((EventTemplate)e1).getMessage().compareToIgnoreCase(((EventTemplate)e2).getMessage());
            else
               result = 0;
				break;
			case EventConfigurator.COLUMN_DESCRIPTION:
				result = ((EventObject)e1).getDescription().compareToIgnoreCase(((EventObject)e2).getDescription());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
