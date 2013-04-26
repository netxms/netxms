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
package org.netxms.ui.eclipse.objectview.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.SoftwarePackage;
import org.netxms.ui.eclipse.objectview.widgets.SoftwareInventory;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Comparator for software package objects
 */
public class SoftwarePackageComparator extends ViewerComparator
{
	private static final long serialVersionUID = 1L;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		final int column = (viewer instanceof SortableTableViewer) ? (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID") : (Integer)((SortableTreeViewer)viewer).getTree().getSortColumn().getData("ID");
		
		if (e1 instanceof SoftwareInventoryNode)
		{
			if (column == 0)
			{
				result = ((SoftwareInventoryNode)e1).getNode().getObjectName().compareToIgnoreCase(((SoftwareInventoryNode)e2).getNode().getObjectName());
			}
			else
			{
				result = 0;
			}
		}
		else
		{
			SoftwarePackage p1 = (SoftwarePackage)e1;
			SoftwarePackage p2 = (SoftwarePackage)e2;
			
			switch(column)
			{
				case SoftwareInventory.COLUMN_DATE:
					result = Long.signum(p1.getInstallDateMs() - p2.getInstallDateMs());
					break;
				case SoftwareInventory.COLUMN_DESCRIPTION:
					result = p1.getDescription().compareToIgnoreCase(p2.getDescription());
					break;
				case SoftwareInventory.COLUMN_NAME:
					result = p1.getName().compareToIgnoreCase(p2.getName());
					break;
				case SoftwareInventory.COLUMN_URL:
					result = p1.getSupportUrl().compareToIgnoreCase(p2.getSupportUrl());
					break;
				case SoftwareInventory.COLUMN_VENDOR:
					result = p1.getVendor().compareToIgnoreCase(p2.getVendor());
					break;
				case SoftwareInventory.COLUMN_VERSION:
					result = p1.getVersion().compareToIgnoreCase(p2.getVersion());
					break;
				default:
					result = 0;
					break;
			}
		}
		int direction = (viewer instanceof SortableTableViewer) ? ((SortableTableViewer)viewer).getTable().getSortDirection() : ((SortableTreeViewer)viewer).getTree().getSortDirection();
		return (direction == SWT.UP) ? result : -result;
	}
}
