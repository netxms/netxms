/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.objects.queries.ObjectQuery;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.ObjectQueryManager;

/**
 * Comparator for object squery list
 */
public class ObjectQueryComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      final ObjectQuery query1 = (ObjectQuery)e1;
      final ObjectQuery query2 = (ObjectQuery)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");

      int result;
      switch(column)
      {
         case ObjectQueryManager.COL_DESCRIPTION:
            result = query1.getDescription().compareToIgnoreCase(query2.getDescription());
            break;
         case ObjectQueryManager.COL_ID:
            result = Integer.signum(query1.getId() - query2.getId());
            break;
         case ObjectQueryManager.COL_IS_VALID:
            result = Boolean.compare(query1.isValid(), query2.isValid());
            break;
         case ObjectQueryManager.COL_NAME:
            result = query1.getName().compareToIgnoreCase(query2.getName());
            break;
         default:
            result = 0;
            break;
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
