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
package org.netxms.ui.eclipse.perfview.widgets.helpers;

import java.text.NumberFormat;
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
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;

/**
 * Label provider for NetXMS table
 */
public class TableLabelProvider extends LabelProvider implements ITableLabelProvider, ITableFontProvider
{
	private TableColumnDefinition[] columns = null;
	private TableRow row;
	private Font keyColumnFont;
   private boolean useMultipliers;
	
	/**
	 * 
	 */
	public TableLabelProvider()
	{
		FontData fd = JFaceResources.getDefaultFont().getFontData()[0];
		fd.setStyle(SWT.BOLD);
		keyColumnFont = new Font(Display.getCurrent(), fd);
	}
	
	/* (non-Javadoc)
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
		row = (TableRow)element;
		
		if (columnIndex >= row.size()) {
			return null;
		}
		else if (useMultipliers) {
		 return getValueForFormat(columnIndex);
		   
		}
		
		return row.get(columnIndex).getValue();
	}

	/* (non-Javadoc)
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
	
	public void useMultipliers(boolean useMultipliers)
	{
	   this.useMultipliers = useMultipliers;
	}
	
	public boolean areMultipliersUsed()
	{
	   return useMultipliers;
	}
	
	/**
	 * @param columnIndex
	 * @return value converted to multiplier form
	 */
	private String getValueForFormat(int columnIndex)
   {
      String value;
      String suffix = null;
      
      try
      {
         switch(columns[columnIndex].getDataType())
         {
            case DataCollectionObject.DT_INT:
            case DataCollectionObject.DT_UINT:
            case DataCollectionItem.DT_INT64:
            case DataCollectionItem.DT_UINT64:               
               if (useMultipliers)
               {
                  long i = Long.parseLong(row.get(columnIndex).getValue());
                  if ((i >= 10000000000000L) || (i <= -10000000000000L))
                  {
                     i = i / 1000000000000L;
                     suffix = "T";
                  }
                  if ((i >= 10000000000L) || (i <= -10000000000L))
                  {
                     i = i / 1000000000L;
                     suffix = "G";
                  }
                  if ((i >= 10000000) || (i <= -10000000))
                  {
                     i = i / 1000000;
                     suffix = "M";
                  }
                  if ((i >= 10000) || (i <= -10000))
                  {
                     i = i / 1000;
                     suffix = "K";
                  }
                  value = Long.toString(i);
               }
               else
               {
                  value = row.get(columnIndex).getValue();
                  suffix = " ";
               }
               break;
            case DataCollectionObject.DT_FLOAT:
               if (useMultipliers)
               {
                  double d = Double.parseDouble(row.get(columnIndex).getValue());
                  NumberFormat nf = NumberFormat.getNumberInstance();
                  nf.setMaximumFractionDigits(2);
                  if ((d >= 10000000000000.0) || (d <= -10000000000000.0))
                  {
                     d = d / 1000000000000.0;
                     suffix = "T";
                  }
                  if ((d >= 10000000000.0) || (d <= -10000000000.0))
                  {
                     d = d / 1000000000.0;
                     suffix = "G";
                  }
                  if ((d >= 10000000) || (d <= -10000000))
                  {
                     d = d / 1000000;
                     suffix = "M";
                  }
                  if ((d >= 10000) || (d <= -10000))
                  {
                     d = d / 1000;
                     suffix = "K";
                  }
                  value = Double.toString(d);
               }
               else
               {
                  value = row.get(columnIndex).getValue();
               }
               break;
            default:
               value = row.get(columnIndex).getValue();
               break;
         }
      }     
      catch(NumberFormatException e)
      {
         value = row.get(columnIndex).getValue();
      }
      
      if (suffix != null)
         return value + " " + suffix;
      return value;
   }
}
