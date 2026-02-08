/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.Widget;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.helpers.TableSortingListener;

/**
 * Implementation of TableViewer with column sorting support
 */
public class SortableTableViewer extends TableViewer
{
	public static final int DEFAULT_STYLE = -1;
	
	private boolean initialized = false;
	private List<TableColumn> columns = new ArrayList<TableColumn>(16);
	private TableSortingListener sortingListener;
	
	/**
    * Constructor
    * 
    * @param parent Parent composite for table control
    * @param names Column names
    * @param widths Column widths (may be null)
    * @param defaultSortingColumn Index of default sorting column
    * @param defaultSortDir default sorting direction
    * @param style widget style
    */
   public SortableTableViewer(Composite parent, String[] names, int[] widths, int defaultSortingColumn, int defaultSortDir, int style)
	{
      this(parent, style);
		createColumns(names, widths, defaultSortingColumn, defaultSortDir);
	}

	/**
    * Constructor for delayed initialization
    * 
    * @param parent Parent composite for table control
    * @param style widget style
    */
	public SortableTableViewer(Composite parent, int style)
	{
		super(new Table(parent, (style == DEFAULT_STYLE) ? (SWT.MULTI | SWT.FULL_SELECTION) : style));
		getTable().setLinesVisible(true);
		getTable().setHeaderVisible(true);
      sortingListener = new TableSortingListener(this);
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

		for(int i = 0; i < names.length; i++)
		{
			TableColumn c = new TableColumn(getTable(), SWT.LEFT);
			columns.add(c);
			c.setText(names[i]);
			if (widths != null)
				c.setWidth(widths[i]);
         c.setData("ID", Integer.valueOf(i));
			c.addSelectionListener(sortingListener);
		}

		if ((defaultSortingColumn >= 0) && (defaultSortingColumn < names.length))
			getTable().setSortColumn(columns.get(defaultSortingColumn));
		getTable().setSortDirection(defaultSortDir);
	}

	/**
	 * Add column to viewer
	 * 
	 * @param name column name
	 * @param width column width
	 * @return created column object
	 */
	public TableColumn addColumn(String name, int width)
	{
	   int index = getTable().getColumnCount();
      TableColumn c = new TableColumn(getTable(), SWT.LEFT);
      columns.add(c);
      c.setText(name);
      c.pack();
      if (width > 0)
         c.setWidth(width);
      c.setData("ID", Integer.valueOf(index));
      c.addSelectionListener(sortingListener);
      return c;
	}

	/**
	 * Get column object by id (named data with key ID)
	 * @param id Column ID
	 * @return Column object or null if object with given ID not found
	 */
	public TableColumn getColumnById(int id)
	{
	   for(TableColumn c : columns)
		{
			if (!c.isDisposed() && ((Integer)c.getData("ID") == id)) //$NON-NLS-1$
			{
				return c;
			}
		}
		return null;
	}
	
	/**
	 * Remove column by ID
	 * 
	 * @param id column ID
	 */
	public void removeColumnById(int id)
	{
      for(TableColumn c : columns)
      {
         if (!c.isDisposed() && ((Integer)c.getData("ID") == id)) //$NON-NLS-1$
         {
            columns.remove(c);
            c.dispose();
            return;
         }
      }
	}

	/**
	 * @return the initialized
	 */
	public boolean isInitialized()
	{
		return initialized;
	}

   /**
    * Reset viewer to uninitialized state
    */
   public void reset()
   {
      initialized = false;
      columns.clear();
      getTable().removeAll();
      for(TableColumn c : getTable().getColumns())
         c.dispose();
   }
	
	/**
	 * Disable sorting
	 */
	public void disableSorting()
	{
      for(TableColumn c : columns)
			c.removeSelectionListener(sortingListener);
		getTable().setSortColumn(null);
	}
	
	/**
	 * Pack columns
	 */
	public void packColumns()
	{
	   Table table = getTable();
	   int count = table.getColumnCount();
	   for(int i = 0; i < count; i++)
      {
         TableColumn c = table.getColumn(i);
         if (c.getResizable())
            c.pack();
      }
	   if (!Registry.IS_WEB_CLIENT)
	      getControl().redraw(); // Fixes display glitch on Windows
	}

   /**
    * Get column index at given point
    * 
    * @param p
    * @return
    */
   public TableColumn getColumnAtPoint(Point p)
   {
      TableItem item = getTable().getItem(p);
      if (item == null)
         return null;
      int columnCount = getTable().getColumnCount();
      for(int i = 0; i < columnCount; i++)
      {
         Rectangle rect = item.getBounds(i);
         if (rect.contains(p))
         {
            return getTable().getColumn(i);
         }
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.TableViewer#getViewerRowFromItem(org.eclipse.swt.widgets.Widget)
    */
	@Override
	public ViewerRow getViewerRowFromItem(Widget item)
	{
		return super.getViewerRowFromItem(item);
	}
}
