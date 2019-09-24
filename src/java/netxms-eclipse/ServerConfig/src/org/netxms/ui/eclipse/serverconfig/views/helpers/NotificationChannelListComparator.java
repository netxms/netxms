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
import org.netxms.client.NotificationChannel;
import org.netxms.ui.eclipse.serverconfig.views.NotificationChannelView;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for notification channel list
 */
public class NotificationChannelListComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		
		NotificationChannel nc1 = (NotificationChannel)e1;
		NotificationChannel nc2 = (NotificationChannel)e2;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case NotificationChannelView.COLUMN_NAME:
				result = nc1.getName().compareToIgnoreCase(nc2.getName());
				break;
         case NotificationChannelView.COLUMN_DRIVER_NAME:
            result = nc1.getDriverName().compareToIgnoreCase(nc2.getDriverName());
            break;
			case NotificationChannelView.COLUMN_DESCRIPTION:
				result = nc1.getDescription().compareToIgnoreCase(nc2.getDescription());
				break;
         case NotificationChannelView.COLUMN_LAST_STATUS:
            result = nc1.getLastStatusAsString().compareToIgnoreCase(nc2.getLastStatusAsString());
            break;
         case NotificationChannelView.COLUMN_ERROR_MESSAGE:
            result = nc1.getErrorMessage().compareToIgnoreCase(nc2.getErrorMessage());
            break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
