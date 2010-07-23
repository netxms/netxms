/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.tools;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;

/**
 * Implementation of TableViewer with column sorting support
 * 
 * @author victor
 *
 */
public class SortableTableViewer extends TableViewer
{
	public static final int DEFAULT_STYLE = -1;
	
	private TableColumn[] columns;
	private TableSortingListener sortingListener;
	
	/**
	 * Constructor
	 * 
	 * @param parent Parent composite for table control
	 * @param names Column names
	 * @param widths Column widths (may be null)
	 * @param defaultSortingColumn Index of default sorting column
	 */
	public SortableTableViewer(Composite parent, String[] names, int[] widths,
	                           int defaultSortingColumn, int defaultSortDir,
	                           int style)
	{
		super(new Table(parent, (style == DEFAULT_STYLE) ? (SWT.MULTI | SWT.FULL_SELECTION) : style));

		sortingListener = new TableSortingListener(this);
		
		columns = new TableColumn[names.length];
		for(int i = 0; i < names.length; i++)
		{
			columns[i] = new TableColumn(getTable(), SWT.LEFT);
			columns[i].setText(names[i]);
			if (widths != null)
				columns[i].setWidth(widths[i]);
			columns[i].setData("ID", new Integer(i));
			columns[i].addSelectionListener(sortingListener);
		}
		getTable().setLinesVisible(true);
		getTable().setHeaderVisible(true);

		getTable().setSortColumn(columns[defaultSortingColumn]);
		getTable().setSortDirection(defaultSortDir);
	}

	/**
	 * Get column object by id (named data with key ID)
	 * @param id Column ID
	 * @return Column object or null if object with given ID not found
	 */
	public TableColumn getColumnById(int id)
	{
		for(int i = 0; i < columns.length; i++)
		{
			if ((Integer)columns[i].getData("ID") == id)
			{
				return columns[i];
			}
		}
		return null;
	}
}
