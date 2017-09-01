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
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Comparator for table items
 */
public class SummaryTableItemComparator extends ViewerComparator
{
   private Table table;
	private int[] formats;
	
	/**
	 * 
	 */
	public SummaryTableItemComparator(Table table)
	{
	   this.table = table;
		this.formats = table.getColumnDataTypes();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final int column = (Integer)((SortableTreeViewer) viewer).getTree().getSortColumn().getData("ID"); //$NON-NLS-1$
		final int format = (column < formats.length) ? formats[column] : DataCollectionItem.DT_STRING;

      final String value1 = getCellValue((TableRow)e1, column);
      final String value2 = getCellValue((TableRow)e2, column);
      
      int result;
		switch(format)
		{
			case DataCollectionItem.DT_INT:
			case DataCollectionItem.DT_UINT:
				result = safeParseInt(value1) - safeParseInt(value2);
				break;
			case DataCollectionItem.DT_INT64:
			case DataCollectionItem.DT_UINT64:
				result = Long.signum(safeParseLong(value1) - safeParseLong(value2));
				break;
			case DataCollectionItem.DT_FLOAT:
				result = (int)Math.signum(safeParseDouble(value1) - safeParseDouble(value2));
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
	
	/**
	 * @param s
	 * @return
	 */
	private static int safeParseInt(String s)
	{
		try
		{
			return Integer.parseInt(s);
		}
		catch(NumberFormatException e)
		{
			return 0;
		}
	}

	/**
	 * @param s
	 * @return
	 */
	private static long safeParseLong(String s)
	{
		try
		{
			return Long.parseLong(s);
		}
		catch(NumberFormatException e)
		{
			return 0;
		}
	}

	/**
	 * @param s
	 * @return
	 */
	private static double safeParseDouble(String s)
	{
		try
		{
			return Double.parseDouble(s);
		}
		catch(NumberFormatException e)
		{
			return 0;
		}
	}
}
