/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.TimeFormatter;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.datacollection.views.HistoricalDataView;

/**
 * Label provider for historical data view
 */
public class HistoricalDataLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private DataFormatter dataFormatter = new DataFormatter();
   private TimeFormatter timeFormatter = DateFormatFactory.getTimeFormatter();

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
      switch(columnIndex)
      {
         case HistoricalDataView.COLUMN_TIMESTAMP:
            return DateFormatFactory.getDateTimeFormat().format(((DciDataRow)element).getTimestamp());
         case HistoricalDataView.COLUMN_VALUE:
            return ((DciDataRow)element).getValueAsString();
         case HistoricalDataView.COLUMN_RAW_VALUE:
            return ((DciDataRow)element).getRawValue();
         case HistoricalDataView.COLUMN_FORMATTED_VALUE:
            return dataFormatter.format(((DciDataRow)element).getValueAsString(), timeFormatter);
      }
      return null;
   }

   /**
    * Set data formatter.
    *
    * @param dataFormatter new data formatter
    */
   public void setDataFormatter(DataFormatter dataFormatter)
   {
      this.dataFormatter = dataFormatter;
   }
}
