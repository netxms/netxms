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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.HardwareComponent;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.ui.eclipse.objectview.widgets.HardwareInventory;

/**
 * Label provider for hardware inventory view
 */
public class HardwareComponentLabelProvider extends LabelProvider implements ITableLabelProvider
{
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      HardwareComponent c = (HardwareComponent)element;
      switch(columnIndex)
      {
         case HardwareInventory.COLUMN_CAPACITY:
            switch(c.getCategory())
            {
               case BATTERY:
                  return Long.toString(c.getCapacity()) + " mWh"; 
               case MEMORY:
               case STORAGE:
                  return new DataFormatter("%*sB", DataType.UINT64, true).format(Long.toString(c.getCapacity())); 
               case PROCESSOR:
                  return new DataFormatter("%*sHz", DataType.UINT64, false).format(Long.toString(c.getCapacity() * 1000000L));
               default:
                  break;
            }
            return "";
         case HardwareInventory.COLUMN_CATEGORY:
            return c.getCategory().toString();
         case HardwareInventory.COLUMN_DESCRIPTION:
            return c.getDescription();
         case HardwareInventory.COLUMN_INDEX:
            return Integer.toString(c.getIndex());
         case HardwareInventory.COLUMN_LOCATION:
            return c.getLocation();
         case HardwareInventory.COLUMN_MODEL:
            return c.getModel();
         case HardwareInventory.COLUMN_PART_NUMBER:
            return c.getPartNumber();
         case HardwareInventory.COLUMN_SERIAL_NUMBER:
            return c.getSerialNumber();
         case HardwareInventory.COLUMN_TYPE:
            return c.getType();
         case HardwareInventory.COLUMN_VENDOR:
            return c.getVendor();
      }      
      return null;
   }
}
