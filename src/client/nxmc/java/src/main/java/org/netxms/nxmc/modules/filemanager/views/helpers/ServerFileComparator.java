/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.filemanager.views.helpers;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.server.ServerFile;
import org.netxms.nxmc.modules.filemanager.views.ServerFileManager;

/**
 * Comparator for ServerFile objects
 */
public class ServerFileComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
		if (sortColumn == null)
			return 0;

		int rc;
		switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
		{
		   case ServerFileManager.COLUMN_NAME:
            rc = ((ServerFile)e1).getName().compareToIgnoreCase(((ServerFile)e2).getName());
            break;
         case ServerFileManager.COLUMN_TYPE:
            rc = ((ServerFile)e1).getExtension().compareToIgnoreCase(((ServerFile)e2).getExtension());
            break;
         case ServerFileManager.COLUMN_SIZE:
            rc = Long.signum(((ServerFile)e1).getSize() - ((ServerFile)e2).getSize());
            break;
         case ServerFileManager.COLUMN_MODIFYED:
            rc = ((ServerFile)e1).getModificationTime().compareTo(((ServerFile)e2).getModificationTime());
            break;
         default:
            rc = 0;
            break;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
