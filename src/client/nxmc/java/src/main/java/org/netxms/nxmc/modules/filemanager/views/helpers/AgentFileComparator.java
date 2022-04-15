/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.client.server.AgentFile;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.modules.filemanager.views.AgentFileManager;

/**
 * Comparator for AgentFile objects
 *
 */
public class AgentFileComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		TreeColumn sortColumn = ((SortableTreeViewer)viewer).getTree().getSortColumn();
		if (sortColumn == null)
			return 0;
		
		int rc;
      switch((Integer)sortColumn.getData("ID"))
		{
		   case AgentFileManager.COLUMN_NAME:
            if(((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory())
            {
               rc = ((AgentFile)e1).getName().compareToIgnoreCase(((AgentFile)e2).getName());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }           
            break;
         case AgentFileManager.COLUMN_TYPE:
            if(((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory())
            {
               rc = ((AgentFile)e1).getExtension().compareToIgnoreCase(((AgentFile)e2).getExtension());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         case AgentFileManager.COLUMN_SIZE:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = Long.signum(((AgentFile)e1).getSize() - ((AgentFile)e2).getSize());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }
            break;
         case AgentFileManager.COLUMN_MODIFYED:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = ((AgentFile)e1).getModificationTime().compareTo(((AgentFile)e2).getModificationTime());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         case AgentFileManager.COLUMN_OWNER:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = ((AgentFile)e1).getOwner().compareTo(((AgentFile)e2).getOwner());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         case AgentFileManager.COLUMN_GROUP:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = ((AgentFile)e1).getGroup().compareTo(((AgentFile)e2).getGroup());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         case AgentFileManager.COLUMN_ACCESS_RIGHTS:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = ((AgentFile)e1).getAccessRights().compareTo(((AgentFile)e2).getAccessRights());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         default:
            rc = 0;
            break;
		}
		int dir = ((SortableTreeViewer)viewer).getTree().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
