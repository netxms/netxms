/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverjobmanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerJob;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.serverjobmanager.views.ServerJobManager;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Server job comparator
 *
 */
public class ServerJobComparator extends ViewerComparator
{
	private NXCSession session;
	
	/**
	 * The constructor.
	 */
	public ServerJobComparator()
	{
		session = (NXCSession)ConsoleSharedData.getSession();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case ServerJobManager.COLUMN_STATUS:
				result = ((ServerJob)e1).getStatus() - ((ServerJob)e2).getStatus();
				break;
			case ServerJobManager.COLUMN_NODE:
				AbstractObject object1 = session.findObjectById(((ServerJob)e1).getNodeId());
				AbstractObject object2 = session.findObjectById(((ServerJob)e2).getNodeId());
				String name1 = (object1 != null) ? object1.getObjectName() : "<unknown>"; //$NON-NLS-1$
				String name2 = (object2 != null) ? object2.getObjectName() : "<unknown>"; //$NON-NLS-1$
				result = name1.compareToIgnoreCase(name2);
				break;
			case ServerJobManager.COLUMN_DESCRIPTION:
				result = ((ServerJob)e1).getDescription().compareToIgnoreCase(((ServerJob)e2).getDescription());
				break;
			case ServerJobManager.COLUMN_MESSAGE:
				result = ((ServerJob)e1).getFailureMessage().compareToIgnoreCase(((ServerJob)e2).getFailureMessage());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.DOWN) ? result : -result;
	}
}
