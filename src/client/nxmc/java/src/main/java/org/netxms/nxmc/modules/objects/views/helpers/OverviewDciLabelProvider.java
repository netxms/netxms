/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.datacollection.DciValue;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;

/**
 * Label provider for overview page DCI list
 */
public class OverviewDciLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == 0)
         return (((DciValue)element).getErrorCount() > 0) ? 
               StatusDisplayInfo.getStatusImage(ObjectStatus.UNKNOWN) : 
                  StatusDisplayInfo.getStatusImage(((DciValue)element).getThresholdSeverity());
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      DciValue dci = (DciValue)element;
      switch(columnIndex)
      {
         case 0:
            return dci.getDescription();
         case 1:
            return dci.getFormattedValue(true, DateFormatFactory.getTimeFormatter());
         case 2:            
            if (dci.getTimestamp().getTime() <= 1000)
               return null;
            return DateFormatFactory.getDateTimeFormat().format(dci.getTimestamp());
         default:
            return null;
      }
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (((DciValue)element).getErrorCount() > 0)
         return ThemeEngine.getForegroundColor("List.Error");
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
