/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.server.ServerVariable;
import org.netxms.nxmc.modules.serverconfig.views.ServerVariables;

/**
 * Comparator for server configuration variables
 */
public class ServerVariableComparator extends ViewerComparator
{
	/**
	 * Compare two booleans and return -1, 0, or 1
	 */
	private static int compareBooleans(boolean b1, boolean b2)
	{
		return (!b1 && b2) ? -1 : ((b1 && !b2) ? 1 : 0);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		
		switch((Integer)((TableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case ServerVariables.COLUMN_NAME:
				result = ((ServerVariable)e1).getName().compareToIgnoreCase(((ServerVariable)e2).getName());
				break;
			case ServerVariables.COLUMN_VALUE:
		      result = ((ServerVariable)e1).getValueForDisplay().compareToIgnoreCase(((ServerVariable)e2).getValueForDisplay());
				break;
			case ServerVariables.COLUMN_DEFAULT_VALUE:
            result = ((ServerVariable)e1).getDefaultValueForDisplay().compareToIgnoreCase(((ServerVariable)e2).getDefaultValueForDisplay());
            break;
			case ServerVariables.COLUMN_NEED_RESTART:
				result = compareBooleans(((ServerVariable)e1).isServerRestartNeeded(), ((ServerVariable)e2).isServerRestartNeeded());
				break;
        case ServerVariables.COLUMN_DESCRIPTION:
            result = ((ServerVariable)e1).getDescription().compareToIgnoreCase(((ServerVariable)e2).getDescription());
            break;
			default:
				result = 0;
				break;
		}
		return (((TableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}