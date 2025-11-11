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

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.HardwareComponent;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.HardwareComponentCategory;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.MeasurementUnit;
import org.netxms.nxmc.modules.objects.widgets.HardwareInventoryTable;

/**
 * Label provider for hardware inventory view
 */
public class HardwareComponentLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private Map<HardwareComponentCategory, String> categoryNames;
   
   /**
    * Constructor
    */
   public HardwareComponentLabelProvider()
   {
      categoryNames = new HashMap<HardwareComponentCategory, String>();
      categoryNames.put(HardwareComponentCategory.BASEBOARD, "Baseboard");
      categoryNames.put(HardwareComponentCategory.BATTERY, "Battery");
      categoryNames.put(HardwareComponentCategory.MEMORY_DEVICE, "Memory device");
      categoryNames.put(HardwareComponentCategory.NETWORK_ADAPTER, "Network adapter");
      categoryNames.put(HardwareComponentCategory.OTHER, "Other");
      categoryNames.put(HardwareComponentCategory.PROCESSOR, "Processor");
      categoryNames.put(HardwareComponentCategory.STORAGE_DEVICE, "Storage device");
   }
   
   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      HardwareComponent c = (HardwareComponent)element;
      DataFormatter df = new DataFormatter().setFormatString("%{u,m}s").setDataType(DataType.UINT64);
      switch(columnIndex)
      {
         case HardwareInventoryTable.COLUMN_CAPACITY:
            switch(c.getCategory())
            {
               case BATTERY:
                  return Long.toString(c.getCapacity()) + " mWh"; 
               case MEMORY_DEVICE:
               case STORAGE_DEVICE:
                  return df.setMeasurementUnit(MeasurementUnit.BYTES_IEC).format(Long.toString(c.getCapacity()), null);
               case NETWORK_ADAPTER:
                  return df.setMeasurementUnit(MeasurementUnit.BPS_METRIC).format(Long.toString(c.getCapacity()), null);
               case PROCESSOR:
                  return df.setMeasurementUnit(MeasurementUnit.HZ).format(Long.toString(c.getCapacity() * 1000000L), null);
               default:
                  break;
            }
            return "";
         case HardwareInventoryTable.COLUMN_CATEGORY:
            return categoryNames.get(c.getCategory());
         case HardwareInventoryTable.COLUMN_DESCRIPTION:
            return c.getDescription();
         case HardwareInventoryTable.COLUMN_INDEX:
            return Integer.toString(c.getIndex());
         case HardwareInventoryTable.COLUMN_LOCATION:
            return c.getLocation();
         case HardwareInventoryTable.COLUMN_MODEL:
            return c.getModel();
         case HardwareInventoryTable.COLUMN_PART_NUMBER:
            return c.getPartNumber();
         case HardwareInventoryTable.COLUMN_SERIAL_NUMBER:
            return c.getSerialNumber();
         case HardwareInventoryTable.COLUMN_TYPE:
            return c.getType();
         case HardwareInventoryTable.COLUMN_VENDOR:
            return c.getVendor();
      }      
      return null;
   }
}
