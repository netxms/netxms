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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.TableRow;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for table items
 *
 */
public class TableItemComparator extends ViewerComparator
{
	private int[] formats;
	
	/**
	 * 
	 */
	public TableItemComparator(int[] formats)
	{
		this.formats = formats;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		final int column = (Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$
		final int format = (column < formats.length) ? formats[column] : DataCollectionItem.DT_STRING;
		
		final String value1 = ((TableRow)e1).get(column).getValue();
		final String value2 = ((TableRow)e2).get(column).getValue();
		
		int result;
		switch(format)
		{
			case DataCollectionItem.DT_STRING:
				result = value1.compareToIgnoreCase(value2);
				break;
			case DataCollectionItem.DT_INT:
				try
				{
					result = Integer.parseInt(value1) - Integer.parseInt(value2);
				}
				catch(NumberFormatException e)
				{
					result = 0;
				}
				break;
			case DataCollectionItem.DT_UINT:
			case DataCollectionItem.DT_INT64:
			case DataCollectionItem.DT_UINT64:
				try
				{
					result = Long.signum(Long.parseLong(value1) - Long.parseLong(value2));
				}
				catch(NumberFormatException e)
				{
					result = 0;
				}
				break;
			case DataCollectionItem.DT_FLOAT:
				try
				{
					result = (int)Math.signum(Double.parseDouble(value1) - Double.parseDouble(value2));
				}
				catch(NumberFormatException e)
				{
					result = 0;
				}
				break;
			default:
				result = 0;
				break;
		}
		
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
