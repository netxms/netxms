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
package org.netxms.ui.eclipse.usermanager;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.api.client.users.User;
import org.netxms.api.client.users.UserGroup;
import org.netxms.ui.eclipse.usermanager.views.UserManagementView;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for user objects
 *
 */
public class UserComparator extends ViewerComparator
{
	/**
	 * Compare object types
	 */
	private int compareTypes(Object o1, Object o2)
	{
		int type1 = (o1 instanceof UserGroup) ? 1 : 0;
		int type2 = (o2 instanceof UserGroup) ? 1 : 0;
		return type1 - type2;
	}

	/**
	 * Get full name for user db object
	 */
	private String getFullName(Object object)
	{
		if (object instanceof User)
			return ((User) object).getFullName();
		return "";
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"))
		{
			case UserManagementView.COLUMN_NAME:
				result = ((AbstractUserObject)e1).getName().compareToIgnoreCase(((AbstractUserObject) e2).getName());
				break;
			case UserManagementView.COLUMN_TYPE:
				result = compareTypes(e1, e2);
				break;
			case UserManagementView.COLUMN_FULLNAME:
				result = getFullName(e1).compareToIgnoreCase(getFullName(e2));
				break;
			case UserManagementView.COLUMN_DESCRIPTION:
				result = ((AbstractUserObject)e1).getDescription().compareToIgnoreCase(((AbstractUserObject)e2).getDescription());
				break;
			case UserManagementView.COLUMN_GUID:
				result = ((AbstractUserObject)e1).getGuid().toString().compareTo(((AbstractUserObject)e2).getGuid().toString());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
