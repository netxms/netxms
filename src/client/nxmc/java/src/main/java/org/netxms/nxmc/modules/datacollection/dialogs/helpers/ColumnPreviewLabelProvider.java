/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.dialogs.helpers;

import java.util.Set;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ITableFontProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;

/**
 * Label provider for column preview table in Create SNMP Table DCI dialog
 */
public class ColumnPreviewLabelProvider extends LabelProvider implements ITableLabelProvider, ITableFontProvider
{
   private Font boldFont;
   private Set<ColumnDefinition> selectedColumns;
   private String dciName;

   /**
    * Create label provider.
    *
    * @param selectedColumns set of selected columns
    * @param dciName OID that will be used as the DCI metric name (bold row indicator)
    */
   public ColumnPreviewLabelProvider(Set<ColumnDefinition> selectedColumns, String dciName)
   {
      FontData fd = JFaceResources.getDefaultFont().getFontData()[0];
      fd.setStyle(SWT.BOLD);
      boldFont = new Font(Display.getCurrent(), fd);
      this.selectedColumns = selectedColumns;
      this.dciName = dciName;
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      boldFont.dispose();
      super.dispose();
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
      ColumnDefinition col = (ColumnDefinition)element;
      switch(columnIndex)
      {
         case 0:
            return selectedColumns.contains(col) ? "\u2713" : "";
         case 1:
            return col.getName();
         case 2:
            return DataCollectionDisplayInfo.getDataTypeName(col.getDataType());
         case 3:
            return (col.getSnmpObjectId() != null) ? col.getSnmpObjectId().toString() : "";
         case 4:
            return col.isInstanceColumn() ? "\u2713" : "";
      }
      return "";
   }

   /**
    * @see org.eclipse.jface.viewers.ITableFontProvider#getFont(java.lang.Object, int)
    */
   @Override
   public Font getFont(Object element, int columnIndex)
   {
      ColumnDefinition col = (ColumnDefinition)element;
      if ((dciName != null) && (col.getSnmpObjectId() != null) && dciName.equals(col.getSnmpObjectId().toString()))
         return boldFont;
      return null;
   }
}
