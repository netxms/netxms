/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.widgets;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.ui.eclipse.widgets.helpers.TableSortingListener;

/**
 * Implementation of TableViewer with column sorting support
 */
public class SortableTableViewer extends TableViewer
{
	private static final long serialVersionUID = 1L;

	public static final int DEFAULT_STYLE = -1;
	
	private boolean initialized = false;
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
		getTable().setLinesVisible(true);
		getTable().setHeaderVisible(true);
		createColumns(names, widths, defaultSortingColumn, defaultSortDir);
	}

	/**
	 * Constructor for delayed initialization
	 * 
	 * @param parent
	 * @param style
	 */
	public SortableTableViewer(Composite parent, int style)
	{
		super(new Table(parent, (style == DEFAULT_STYLE) ? (SWT.MULTI | SWT.FULL_SELECTION) : style));
		getTable().setLinesVisible(true);
		getTable().setHeaderVisible(true);
	}

	/**
	 * Create columns
	 * 
	 * @param names
	 * @param widths
	 * @param defaultSortingColumn
	 * @param defaultSortDir
	 */
	public void createColumns(String[] names, int[] widths, int defaultSortingColumn, int defaultSortDir)
	{
		if (initialized)
			return;
		initialized = true;
		
		sortingListener = new TableSortingListener(this);
		
		columns = new TableColumn[names.length];
		for(int i = 0; i < names.length; i++)
		{
			columns[i] = new TableColumn(getTable(), SWT.LEFT);
			columns[i].setText(names[i]);
			if (widths != null)
				columns[i].setWidth(widths[i]);
			columns[i].setData("ID", new Integer(i)); //$NON-NLS-1$
			columns[i].addSelectionListener(sortingListener);
		}

		if ((defaultSortingColumn >= 0) && (defaultSortingColumn < names.length))
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
			if ((Integer)columns[i].getData("ID") == id) //$NON-NLS-1$
			{
				return columns[i];
			}
		}
		return null;
	}

	/**
	 * @return the initialized
	 */
	public boolean isInitialized()
	{
		return initialized;
	}
	
	/**
	 * Disable sorting
	 */
	public void disableSorting()
	{
		for(int i = 0; i < columns.length; i++)
			columns[i].removeSelectionListener(sortingListener);
		getTable().setSortColumn(null);
	}
}
