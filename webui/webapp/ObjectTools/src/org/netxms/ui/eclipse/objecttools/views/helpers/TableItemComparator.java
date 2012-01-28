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
package org.netxms.ui.eclipse.objecttools.views.helpers;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.List;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.objecttools.ObjectToolTableColumn;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for table items
 *
 */
public class TableItemComparator extends ViewerComparator
{
	private static final long serialVersionUID = 1L;

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
		final int format = (column < formats.length) ? formats[column] : ObjectToolTableColumn.FORMAT_STRING;
		
		final String value1 = ((List<String>)e1).get(column);
		final String value2 = ((List<String>)e2).get(column);
		
		int result;
		switch(format)
		{
			case ObjectToolTableColumn.FORMAT_STRING:
			case ObjectToolTableColumn.FORMAT_MAC_ADDR:
			case ObjectToolTableColumn.FORMAT_IFINDEX:
				result = value1.compareToIgnoreCase(value2);
				break;
			case ObjectToolTableColumn.FORMAT_INTEGER:
				try
				{
					result = Integer.parseInt(value1) - Integer.parseInt(value2);
				}
				catch(NumberFormatException e)
				{
					result = 0;
				}
				break;
			case ObjectToolTableColumn.FORMAT_FLOAT:
				try
				{
					result = (int)Math.signum(Double.parseDouble(value1) - Double.parseDouble(value2));
				}
				catch(NumberFormatException e)
				{
					result = 0;
				}
				break;
			case ObjectToolTableColumn.FORMAT_IP_ADDR:
				try
				{
					byte[] addr1 = InetAddress.getByName(value1).getAddress();
					byte[] addr2 = InetAddress.getByName(value1).getAddress();

					result = 0;
					for(int i = 0; (i < addr1.length) && (result == 0); i++)
					{
						result = addr1[i] - addr2[i];
					}
				}
				catch(UnknownHostException e)
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
