/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.HardwareComponent;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.widgets.HardwareInventoryTable;

/**
 * Comparator for hardware component objects
 */
public class HardwareComponentComparator extends ViewerComparator
{

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      HardwareComponent c1 = (HardwareComponent)e1;
      HardwareComponent c2 = (HardwareComponent)e2;
      int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
      
      int result = 0;
      switch(column)
      {
         case HardwareInventoryTable.COLUMN_CAPACITY:
            result = Long.signum(c1.getCapacity() - c2.getCapacity());
            break;
         case HardwareInventoryTable.COLUMN_CATEGORY:
            result = c1.getCategory().getValue() - c2.getCategory().getValue();
            break;
         case HardwareInventoryTable.COLUMN_DESCRIPTION:
            result = c1.getDescription().compareToIgnoreCase(c2.getDescription());
            break;
         case HardwareInventoryTable.COLUMN_INDEX:
            result = Integer.signum(c1.getIndex() - c2.getIndex());
            break;
         case HardwareInventoryTable.COLUMN_LOCATION:
            result = c1.getLocation().compareToIgnoreCase(c2.getLocation());
            break;
         case HardwareInventoryTable.COLUMN_MODEL:
            result = c1.getModel().compareToIgnoreCase(c2.getModel());
            break;
         case HardwareInventoryTable.COLUMN_PART_NUMBER:
            result = c1.getPartNumber().compareToIgnoreCase(c2.getPartNumber());
            break;
         case HardwareInventoryTable.COLUMN_SERIAL_NUMBER:
            result = c1.getSerialNumber().compareToIgnoreCase(c2.getSerialNumber());
            break;
         case HardwareInventoryTable.COLUMN_TYPE:
            result = c1.getType().compareToIgnoreCase(c2.getType());
            break;
         case HardwareInventoryTable.COLUMN_VENDOR:
            result = c1.getVendor().compareToIgnoreCase(c2.getVendor());
            break;
      }
      
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
   
}
