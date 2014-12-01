/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import org.eclipse.jface.viewers.ITableColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.TableCell;
import org.netxms.client.TableRow;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * Label provider for NetXMS table
 */
public class TableLabelProvider extends LabelProvider implements ITableLabelProvider, ITableColorProvider
{
   private static final Color FOREGROUND_COLOR_DARK = new Color(Display.getCurrent(), 0, 0, 0);
   private static final Color FOREGROUND_COLOR_LIGHT = new Color(Display.getCurrent(), 255, 255, 255);
   private static final Color[] FOREGROUND_COLORS =
      { null, FOREGROUND_COLOR_DARK, FOREGROUND_COLOR_DARK, FOREGROUND_COLOR_LIGHT, FOREGROUND_COLOR_LIGHT };

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
		TableRow row = (TableRow)element;
		
		if (columnIndex >= row.size())
			return null;
		
		return row.get(columnIndex).getValue();
	}

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableColorProvider#getForeground(java.lang.Object, int)
    */
   @Override
   public Color getForeground(Object element, int columnIndex)
   {
      TableRow row = (TableRow)element;
      
      if (columnIndex >= row.size())
         return null;
      
      TableCell cell = row.get(columnIndex);
      return (cell.getStatus() >= 0) && (cell.getStatus() < FOREGROUND_COLORS.length) ? FOREGROUND_COLORS[cell.getStatus()] : null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableColorProvider#getBackground(java.lang.Object, int)
    */
   @Override
   public Color getBackground(Object element, int columnIndex)
   {
      TableRow row = (TableRow)element;
      
      if (columnIndex >= row.size())
         return null;
      
      TableCell cell = row.get(columnIndex);
      return (cell.getStatus() > 0) ? StatusDisplayInfo.getStatusColor(cell.getStatus()) : null;
   }
}
