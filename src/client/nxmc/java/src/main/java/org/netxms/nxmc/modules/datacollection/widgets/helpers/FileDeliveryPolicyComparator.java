/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import java.util.Date;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.modules.datacollection.widgets.FileDeliveryPolicyEditor;

/**
 * File delivery policy comparator
 */
public final class FileDeliveryPolicyComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      int dir = ((SortableTreeViewer)viewer).getTree().getSortDirection();
      PathElement p1 = (PathElement)e1;
      PathElement p2 = (PathElement)e2;
      if (p1.isFile() && !p2.isFile())
         return (dir == SWT.UP) ? 1 : -1;
      if (!p1.isFile() && p2.isFile())
         return (dir == SWT.UP) ? -1 : 1;
      
      TreeColumn sortColumn = ((SortableTreeViewer)viewer).getTree().getSortColumn();
      if (sortColumn == null)
         return 0;
      
      int rc = 0;
      switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
      {
         case FileDeliveryPolicyEditor.COLUMN_NAME:
            rc = p1.getName().compareToIgnoreCase(p2.getName());
         case FileDeliveryPolicyEditor.COLUMN_GUID:
            rc = p1.getGuid() != null ? p1.getGuid().compareTo(p2.getGuid()) : 0;
         case FileDeliveryPolicyEditor.COLUMN_DATE:
            if (!p1.isFile())
               rc = 0;
            else
            {
               Date date1 = p1.getCreationTime();
               Date date2 = p2.getCreationTime();
               if (date1 == null)
                  if (date2 == null)
                     rc = 0;
                  else
                     rc = 1;
               else
                  if (date2 == null)
                     rc = -1;
                  else
                     rc = date1.compareTo(date2);
            }
      }
      return (dir == SWT.UP) ? rc : -rc;
   }
}