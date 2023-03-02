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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.AgentPolicy;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.datacollection.views.PolicyListView;

/**
 * Policy comparator
 */
public class PolicyComparator extends ViewerComparator
{	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		AgentPolicy policy1 = (AgentPolicy)e1;
		AgentPolicy policy2 = (AgentPolicy)e2;

		int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
		switch(column)
		{
			case PolicyListView.COLUMN_NAME:
				result = policy1.getName().compareTo(policy2.getName());
				break;
			case PolicyListView.COLUMN_TYPE:
				result = policy1.getPolicyType().compareToIgnoreCase(policy2.getPolicyType());
				break;
			case PolicyListView.COLUMN_GUID:
				result = policy1.getGuid().compareTo(policy2.getGuid());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
