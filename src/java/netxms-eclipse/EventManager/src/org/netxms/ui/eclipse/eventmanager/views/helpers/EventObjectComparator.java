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
import org.netxms.client.events.EventGroup;
import org.netxms.client.events.EventObject;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.eventmanager.widgets.EventObjectList;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Event template comparator
 */
public class EventObjectComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		switch((Integer)((SortableTreeViewer)viewer).getTree().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case EventObjectList.COLUMN_CODE:
			   if ((e1 instanceof EventGroup) == (e2 instanceof EventGroup))
	            result = (int)(((EventObject)e1).getCode() - ((EventObject)e2).getCode());
			   else
			      result = (e1 instanceof EventGroup) ? -1 : 1;
				break;
			case EventObjectList.COLUMN_NAME:
            if ((e1 instanceof EventGroup) == (e2 instanceof EventGroup))
               result = ((EventObject)e1).getName().compareToIgnoreCase(((EventObject)e2).getName());
            else
               result = (e1 instanceof EventGroup) ? -1 : 1;
				break;
			case EventObjectList.COLUMN_SEVERITY:
			   if (e1 instanceof EventTemplate && e2 instanceof EventTemplate)
			      result = ((EventTemplate)e1).getSeverity().compareTo(((EventTemplate)e2).getSeverity());
			   else
			      result = 0;
				break;
			case EventObjectList.COLUMN_FLAGS:
            if (e1 instanceof EventTemplate && e2 instanceof EventTemplate)
               result = ((EventTemplate)e1).getFlags() - ((EventTemplate)e2).getFlags();
            else
               result = 0;
				break;
			case EventObjectList.COLUMN_MESSAGE:
            if (e1 instanceof EventTemplate && e2 instanceof EventTemplate)
               result = ((EventTemplate)e1).getMessage().compareToIgnoreCase(((EventTemplate)e2).getMessage());
            else
               result = 0;
				break;
			case EventObjectList.COLUMN_DESCRIPTION:
            if ((e1 instanceof EventGroup) == (e2 instanceof EventGroup))
               result = ((EventObject)e1).getDescription().compareToIgnoreCase(((EventObject)e2).getDescription());
            else
               result = (e1 instanceof EventGroup) ? -1 : 1;
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTreeViewer)viewer).getTree().getSortDirection() == SWT.UP) ? result : -result;
	}
}
