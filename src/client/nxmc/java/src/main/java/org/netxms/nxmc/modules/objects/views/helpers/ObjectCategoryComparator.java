/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Victor Kirhenshtein
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
import org.netxms.base.NXCommon;
import org.netxms.client.objects.ObjectCategory;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.ObjectCategoryManager;

/**
 * Comparator for object category list
 */
public class ObjectCategoryComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      ObjectCategory c1 = (ObjectCategory)e1;
      ObjectCategory c2 = (ObjectCategory)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$

      int result;
      switch(column)
      {
         case ObjectCategoryManager.COL_ICON:
            result = ((c1.getIcon() != null) ? c1.getIcon() : NXCommon.EMPTY_GUID).compareTo((c2.getIcon() != null) ? c2.getIcon() : NXCommon.EMPTY_GUID);
            break;
         case ObjectCategoryManager.COL_ID:
            result = c1.getId() - c2.getId();
            break;
         case ObjectCategoryManager.COL_MAP_IMAGE:
            result = ((c1.getMapImage() != null) ? c1.getMapImage() : NXCommon.EMPTY_GUID).compareTo((c2.getMapImage() != null) ? c2.getMapImage() : NXCommon.EMPTY_GUID);
            break;
         case ObjectCategoryManager.COL_NAME:
            result = c1.getName().compareToIgnoreCase(c2.getName());
            break;
         default:
            result = 0;
            break;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
