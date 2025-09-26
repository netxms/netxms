/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ITableFontProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.TableRow;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.nxmc.localization.DateFormatFactory;

/**
 * Label provider for NetXMS table
 */
public class TableLabelProvider extends LabelProvider implements ITableLabelProvider, ITableFontProvider
{
	private TableColumnDefinition[] columns = null;
	private Font keyColumnFont;
   private boolean useMultipliers = true;
	
	/**
	 * 
	 */
	public TableLabelProvider()
	{
		FontData fd = JFaceResources.getDefaultFont().getFontData()[0];
		fd.setStyle(SWT.BOLD);
		keyColumnFont = new Font(Display.getCurrent(), fd);
	}
	
   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
	@Override
	public void dispose()
	{
		keyColumnFont.dispose();
		super.dispose();
	}

	/**
	 * @param tableColumnDefinitions the columns to set
	 */
	public void setColumns(TableColumnDefinition[] columns)
	{
		this.columns = columns;
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
      TableRow row = (TableRow)element;

      if (columnIndex >= row.size())
         return null;
      return new DataFormatter().setDataType(columns[columnIndex].getDataType())
            .setMeasurementUnit(columns[columnIndex].getMeasurementUnit())
            .setMultiplierPower(columns[columnIndex].getMultiplierPower())
            .setUseMultipliers(columns[columnIndex].getUseMultiplier())
            .setDefaultForMultipliers(useMultipliers)
            .format(row.get(columnIndex).getValue(), DateFormatFactory.getTimeFormatter());
	}

   /**
    * @see org.eclipse.jface.viewers.ITableFontProvider#getFont(java.lang.Object, int)
    */
	@Override
	public Font getFont(Object element, int columnIndex)
	{
		if ((columns == null) || (columnIndex >= columns.length))
			return null;
		if (columns[columnIndex].isInstanceColumn())
			return keyColumnFont;
		return null;
	}
	
	/**
	 * @param useMultipliers
	 */
	public void setUseMultipliers(boolean useMultipliers)
	{
	   this.useMultipliers = useMultipliers;
	}
	
	/**
	 * @return
	 */
	public boolean areMultipliersUsed()
	{
	   return useMultipliers;
	}
}
