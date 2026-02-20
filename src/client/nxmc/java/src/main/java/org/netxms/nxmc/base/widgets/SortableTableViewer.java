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
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.ScrollBar;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.Widget;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.helpers.TableSortingListener;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Implementation of TableViewer with column sorting support
 */
public class SortableTableViewer extends TableViewer
{
	public static final int DEFAULT_STYLE = -1;

	private boolean initialized = false;
	private List<TableColumn> columns = new ArrayList<TableColumn>(16);
	private TableSortingListener sortingListener;
	private Action actionResetColumnOrder;
	private Action actionShowAllColumns;
	private Menu headerMenu;
	private int clickedColumnId = -1;

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
    * Enable column reordering with persistence.
    */
   public void enableColumnReordering()
   {
      enableColumnReordering(true);
   }

   /**
    * Enable column reordering.
    *
    * @param persist if true, column order is saved/restored between restarts
    */
   public void enableColumnReordering(boolean persist)
   {
      final I18n i18n = LocalizationHelper.getI18n(SortableTableViewer.class);
      Table table = getTable();
      for(TableColumn c : table.getColumns())
         c.setMoveable(true);
      table.setData("persistColumnOrder", Boolean.valueOf(persist));

      actionResetColumnOrder = new Action(i18n.tr("Restore Default Column Order")) {
         @Override
         public void run()
         {
            resetColumnOrder();
         }
      };

      actionShowAllColumns = new Action(i18n.tr("Show All Columns")) {
         @Override
         public void run()
         {
            showAllColumns();
         }
      };

      headerMenu = new Menu(table);
      headerMenu.addListener(SWT.Show, e -> {
         for(MenuItem item : headerMenu.getItems())
            item.dispose();

         int visibleCount = 0;
         for(TableColumn c : table.getColumns())
            if (c.getData("savedWidth") == null)
               visibleCount++;

         if (clickedColumnId >= 0)
         {
            TableColumn clickedColumn = getColumnById(clickedColumnId);
            if (clickedColumn != null && clickedColumn.getData("savedWidth") == null)
            {
               MenuItem hideItem = new MenuItem(headerMenu, SWT.PUSH);
               hideItem.setText(i18n.tr("Hide \"{0}\"", clickedColumn.getText()));
               hideItem.addListener(SWT.Selection, ev -> hideColumn(clickedColumnId));
               hideItem.setEnabled(visibleCount > 1);
            }
         }

         if (hasHiddenColumns())
         {
            MenuItem showAllItem = new MenuItem(headerMenu, SWT.PUSH);
            showAllItem.setText(i18n.tr("Show All Columns"));
            showAllItem.addListener(SWT.Selection, ev -> showAllColumns());

            MenuItem showCascade = new MenuItem(headerMenu, SWT.CASCADE);
            showCascade.setText(i18n.tr("Show Column"));
            Menu showMenu = new Menu(headerMenu);
            showCascade.setMenu(showMenu);

            int[] order = table.getColumnOrder();
            for(int idx : order)
            {
               TableColumn c = table.getColumn(idx);
               if (c.getData("savedWidth") != null)
               {
                  int colId = (Integer)c.getData("ID");
                  MenuItem showItem = new MenuItem(showMenu, SWT.PUSH);
                  showItem.setText(c.getText());
                  showItem.addListener(SWT.Selection, ev -> showColumn(colId));
               }
            }
         }

         new MenuItem(headerMenu, SWT.SEPARATOR);
         MenuItem resetItem = new MenuItem(headerMenu, SWT.PUSH);
         resetItem.setText(i18n.tr("Restore Default Column Order"));
         resetItem.addListener(SWT.Selection, ev -> resetColumnOrder());
      });

      table.addListener(SWT.MenuDetect, event -> {
         Point pt = table.getDisplay().map(null, table, new Point(event.x, event.y));
         if (pt.y < table.getHeaderHeight())
         {
            clickedColumnId = getColumnIdAtHeaderPoint(pt);
            headerMenu.setLocation(event.x, event.y);
            headerMenu.setVisible(true);
            event.doit = false;
         }
      });
   }

   /**
    * Reset column order to default sequential order.
    */
   public void resetColumnOrder()
   {
      Table table = getTable();
      int count = table.getColumnCount();
      int[] order = new int[count];
      for(int i = 0; i < count; i++)
         order[i] = i;
      table.setColumnOrder(order);
   }

   /**
    * Hide column by ID (save width, set width to 0, make non-resizable).
    *
    * @param columnId column ID
    */
   public void hideColumn(int columnId)
   {
      TableColumn column = getColumnById(columnId);
      if (column == null || column.getData("savedWidth") != null)
         return;
      column.setData("savedWidth", Integer.valueOf(column.getWidth()));
      column.setWidth(0);
      column.setResizable(false);
   }

   /**
    * Show column by ID (restore width, make resizable, clear saved width).
    *
    * @param columnId column ID
    */
   public void showColumn(int columnId)
   {
      TableColumn column = getColumnById(columnId);
      if (column == null || column.getData("savedWidth") == null)
         return;
      int width = (Integer)column.getData("savedWidth");
      column.setData("savedWidth", null);
      column.setResizable(true);
      if (width > 0)
         column.setWidth(width);
      else
         column.pack();
   }

   /**
    * Show all hidden columns.
    */
   public void showAllColumns()
   {
      for(TableColumn c : getTable().getColumns())
      {
         if (c.getData("savedWidth") != null)
         {
            int id = (Integer)c.getData("ID");
            showColumn(id);
         }
      }
   }

   /**
    * Check if any columns are hidden.
    *
    * @return true if there are hidden columns
    */
   public boolean hasHiddenColumns()
   {
      for(TableColumn c : getTable().getColumns())
         if (c.getData("savedWidth") != null)
            return true;
      return false;
   }

   /**
    * Get column ID at the given header point by walking columns in display order.
    *
    * @param pt point relative to the table
    * @return column ID or -1 if not found
    */
   private int getColumnIdAtHeaderPoint(Point pt)
   {
      Table table = getTable();
      int[] order = table.getColumnOrder();
      int scrollOffset = 0;
      ScrollBar hBar = table.getHorizontalBar();
      if (hBar != null)
         scrollOffset = hBar.getSelection();
      int x = scrollOffset;
      for(int idx : order)
      {
         TableColumn c = table.getColumn(idx);
         int w = c.getWidth();
         if (pt.x >= x - scrollOffset && pt.x < x - scrollOffset + w)
         {
            Object id = c.getData("ID");
            return (id instanceof Integer) ? (Integer)id : -1;
         }
         x += w;
      }
      return -1;
   }

   /**
    * Get action for resetting column order to default. Returns null if column reordering is not enabled.
    *
    * @return action for resetting column order or null
    */
   public Action getResetColumnOrderAction()
   {
      return actionResetColumnOrder;
   }

   /**
    * Get action for showing all hidden columns. Returns null if column reordering is not enabled.
    *
    * @return action for showing all hidden columns or null
    */
   public Action getShowAllColumnsAction()
   {
      return actionShowAllColumns;
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
