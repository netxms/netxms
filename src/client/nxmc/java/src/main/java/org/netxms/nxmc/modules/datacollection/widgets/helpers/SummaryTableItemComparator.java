/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.constants.DataType;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;

/**
 * Comparator for table items
 */
public class SummaryTableItemComparator extends ViewerComparator
{
   private Table table;
	private DataType[] formats;
	
	/**
	 * 
	 */
	public SummaryTableItemComparator(Table table)
	{
	   this.table = table;
		this.formats = table.getColumnDataTypes();
	}

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final int column = (Integer)((SortableTreeViewer) viewer).getTree().getSortColumn().getData("ID"); //$NON-NLS-1$
		final DataType format = (column < formats.length) ? formats[column] : DataType.STRING;

      final String value1 = getCellValue((TableRow)e1, column);
      final String value2 = getCellValue((TableRow)e2, column);
      
      int result;
		switch(format)
		{
			case INT32:
			case UINT32:
			case COUNTER32:
			case INT64:
			case UINT64:
			case COUNTER64:
			case FLOAT:
				try
				{
					result = (int)Math.signum(Double.parseDouble(value1) - Double.parseDouble(value2));
				}
				catch(NumberFormatException e)
				{
					result = value1.compareToIgnoreCase(value2);
				}
				break;
			default:
				result = value1.compareToIgnoreCase(value2);
				break;
		}
		return (((SortableTreeViewer)viewer).getTree().getSortDirection() == SWT.UP) ? result : -result;
	}
	
	private String getCellValue(TableRow r, int column)
	{
      String value = r.get(column).getValue();
      if (((value == null) || value.isEmpty()) && (r.getBaseRow() != -1))
        return table.getCellValue(r.getBaseRow(), column);
      return value;
	}
}
