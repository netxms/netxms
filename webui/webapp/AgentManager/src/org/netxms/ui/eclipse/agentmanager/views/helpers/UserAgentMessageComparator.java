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
package org.netxms.ui.eclipse.agentmanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.UserAgentMessage;
import org.netxms.ui.eclipse.agentmanager.views.UserAgentMessagesView;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * User agent message comparator
 */
public class UserAgentMessageComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
	   UserAgentMessage uam1 = (UserAgentMessage)e1;
	   UserAgentMessage uam2 = (UserAgentMessage)e2;
		int result;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case UserAgentMessagesView.COL_ID:
				result = Long.signum(uam1.getId() - uam2.getId());
				break;
			case UserAgentMessagesView.COL_OBJECTS:
				result = uam1.getObjectNamesAsString().compareToIgnoreCase(uam2.getObjectNamesAsString());
				break;
			case UserAgentMessagesView.COL_MESSAGE:
				result = uam1.getMessage().compareTo(uam2.getMessage());
				break;
			case UserAgentMessagesView.COL_IS_RECALLED:
				result = Boolean.compare(uam1.isRecalled(), uam2.isRecalled());
				break;
			case UserAgentMessagesView.COL_START_TIME:
				result = uam1.getStartTime().compareTo(uam2.getStartTime());
				break;
			case UserAgentMessagesView.COL_END_TIME:
            result = uam1.getEndTime().compareTo(uam2.getEndTime());
				break;
			default:
				result = 0;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
