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
package org.netxms.ui.eclipse.objectview.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.HardwareComponent;
import org.netxms.ui.eclipse.objectview.widgets.HardwareInventory;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

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
         case HardwareInventory.COLUMN_CAPACITY:
            result = Integer.signum(c1.getCapacity() - c2.getCapacity());
            break;
         case HardwareInventory.COLUMN_INDEX:
            result = Integer.signum(c1.getIndex() - c2.getIndex());
            break;
         case HardwareInventory.COLUMN_MODEL:
            result = c1.getModel().compareToIgnoreCase(c2.getModel());
            break;
         case HardwareInventory.COLUMN_SERIAL:
            result = c1.getSerial().compareToIgnoreCase(c2.getSerial());
            break;
         case HardwareInventory.COLUMN_TYPE:
            result = c1.getType().compareToIgnoreCase(c2.getType());
            break;
         case HardwareInventory.COLUMN_VENDOR:
            result = c1.getVendor().compareToIgnoreCase(c2.getVendor());
            break;
      }
      

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
   
}
