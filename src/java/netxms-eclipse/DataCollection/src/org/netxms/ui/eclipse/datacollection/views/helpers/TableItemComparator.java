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
package org.netxms.ui.eclipse.datacollection.views.helpers;

import java.util.List;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for table items
 */
public class TableItemComparator extends ViewerComparator
{
	private Integer[] formats;
	
	/**
	 * 
	 */
	public TableItemComparator(Integer[] formats)
	{
		this.formats = formats;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID");
		final int format = (column < formats.length) ? formats[column] : DataCollectionItem.DT_STRING;
		
		final String value1 = ((List<String>)e1).get(column);
		final String value2 = ((List<String>)e2).get(column);
		
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
		
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
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
