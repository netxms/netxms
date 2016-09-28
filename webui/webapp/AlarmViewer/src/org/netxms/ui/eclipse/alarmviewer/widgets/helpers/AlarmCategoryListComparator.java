/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.ui.eclipse.alarmviewer.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.jface.viewers.TableViewer;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmCategoryList;
import org.netxms.client.events.AlarmCategory;

/**
 * Comparator for Alarm Category List
 */
public class AlarmCategoryListComparator extends ViewerComparator
{
   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
      if (sortColumn == null)
         return 0;

      int rc;
      switch((Integer)sortColumn.getData("ID"))
      {
         case AlarmCategoryList.COLUMN_ID:
            rc = Long.signum(((AlarmCategory)e1).getId() - ((AlarmCategory)e2).getId());
            break;
         case AlarmCategoryList.COLUMN_NAME:
            rc = (((AlarmCategory)e1).getName().compareToIgnoreCase(((AlarmCategory)e2).getName()));
            break;
         case AlarmCategoryList.COLUMN_DESCRIPTION:
            rc = (((AlarmCategory)e1).getDescription().compareToIgnoreCase(((AlarmCategory)e2).getDescription()));
            break;
         default:
            rc = 0;
            break;
      }
      int dir = ((TableViewer)viewer).getTable().getSortDirection();
      return (dir == SWT.UP) ? rc : -rc;
   }
}
