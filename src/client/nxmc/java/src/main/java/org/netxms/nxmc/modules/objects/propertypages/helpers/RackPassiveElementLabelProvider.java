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
package org.netxms.nxmc.modules.objects.propertypages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.nxmc.modules.objects.propertypages.RackPassiveElements;

/**
 * Label provider for list of passive rack elements
 */
public class RackPassiveElementLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final static String[] ORIENTATION = { "Fill", "Front", "Rear" };
   private final static String[] TYPE = { "Patch panel", "Filler panel", "Organiser", "PDU" };

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      PassiveRackElement entry = (PassiveRackElement)element;
      switch(columnIndex)
      {
         case RackPassiveElements.COLUMN_HEIGHT:
            return Integer.toString(entry.getHeight());
         case RackPassiveElements.COLUMN_NAME:
            return entry.getName();
         case RackPassiveElements.COLUMN_ORIENTATION:
            return ORIENTATION[entry.getOrientation().getValue()];
         case RackPassiveElements.COLUMN_POSITION:
            return Integer.toString(entry.getPosition());
         case RackPassiveElements.COLUMN_TYPE:
            return TYPE[entry.getType().getValue()];
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }
}
