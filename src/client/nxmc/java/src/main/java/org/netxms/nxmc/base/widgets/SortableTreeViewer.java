/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.ScrollBar;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeColumn;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.helpers.TreeSortingListener;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Implementation of TreeViewer with column sorting support
 */
public class SortableTreeViewer extends TreeViewer
{
	public static final int DEFAULT_STYLE = -1;

	private boolean initialized = false;
   private List<TreeColumn> columns;
	private TreeSortingListener sortingListener;
	private Action actionResetColumnOrder;
	private Action actionShowAllColumns;
	private Action actionAutoSizeColumns;
	private Menu headerMenu;
	private int clickedColumnId = -1;
   private String configPrefix;
	private boolean autoResizeEnabled = true;

   /**
    * Constructor for delayed initialization
    *
    * @param parent
    * @param style
    */
   public SortableTreeViewer(Composite parent, int style)
   {
      this(parent, null, null, 0, -1, style, null);
   }

   /**
    * Constructor
    *
    * @param parent Parent composite for table control
    * @param names Column names
    * @param widths Column widths (may be null)
    * @param defaultSortingColumn Index of default sorting column
    */
   public SortableTreeViewer(Composite parent, String[] names, int[] widths, int defaultSortingColumn, int defaultSortDir, int style)
   {
      this(parent, names, widths, defaultSortingColumn, defaultSortDir, style, null);
   }

   /**
    * Constructor with automatic persistence of column settings and toggleable auto-resize.
    * When a configuration prefix is provided, the viewer restores saved column widths, order, visibility
    * and sort state on creation, saves them on dispose, and enables column reordering.
    *
    * @param parent Parent composite for tree control
    * @param names Column names
    * @param widths Default column widths (overridden by saved values if present)
    * @param defaultSortingColumn Index of default sorting column
    * @param defaultSortDir default sorting direction
    * @param style widget style
    * @param configPrefix preference store prefix for persisting column settings
    */
   public SortableTreeViewer(Composite parent, String[] names, int[] widths, int defaultSortingColumn, int defaultSortDir, int style, String configPrefix)
   {
      super(new Tree(parent, (style == DEFAULT_STYLE) ? (SWT.MULTI | SWT.FULL_SELECTION) : style));
      getTree().setLinesVisible(true);
      getTree().setHeaderVisible(true);
      sortingListener = new TreeSortingListener(this);
      if (names != null)
      {
         createColumns(names, widths, defaultSortingColumn, defaultSortDir);
         if (configPrefix != null)
            enablePersistence(configPrefix);
      }
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

      columns = new ArrayList<TreeColumn>(names.length);
		for(int i = 0; i < names.length; i++)
		{
			TreeColumn c = new TreeColumn(getTree(), SWT.LEFT);
			c.setText(names[i]);
			if (widths != null)
				c.setWidth(widths[i]);
         c.setData("ID", Integer.valueOf(i));
			c.addSelectionListener(sortingListener);
			columns.add(c);
		}

		if ((defaultSortingColumn >= 0) && (defaultSortingColumn < names.length))
			getTree().setSortColumn(columns.get(defaultSortingColumn));
		getTree().setSortDirection(defaultSortDir);
	}

   /**
    * @see org.eclipse.jface.viewers.StructuredViewer#inputChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void inputChanged(Object input, Object oldInput)
   {
      super.inputChanged(input, oldInput);
      packColumns(false);
   }

   /**
    * @see org.eclipse.jface.viewers.AbstractTreeViewer#internalRefresh(java.lang.Object, boolean)
    */
   @Override
   protected void internalRefresh(Object element, boolean updateLabels)
   {
      super.internalRefresh(element, updateLabels);
      // refresh() in viewers using setInput() once and updating data via refresh() (e.g. AlarmList)
      // would otherwise leave columns at their initial (often empty-input) packed widths
      packColumns(false);
   }

   /**
    * Pack columns unconditionally (equivalent to {@link #packColumns(boolean) packColumns(true)}).
    */
   public void packColumns()
   {
      packColumns(true);
   }

   /**
    * Pack columns. When <code>force</code> is <code>false</code>, columns are packed only if automatic
    * column resize is enabled on this viewer; this allows callers in refresh paths to respect the user's
    * "Resize columns automatically" preference.
    *
    * @param force if true, pack columns regardless of auto-resize preference
    */
   public void packColumns(boolean force)
   {
      if (!force && !autoResizeEnabled)
         return;

      Tree tree = getTree();
      int count = tree.getColumnCount();
      for(int i = 0; i < count; i++)
      {
         TreeColumn c = tree.getColumn(i);
         if (c.getResizable())
         {
            // setWidth(0) forces SWT to drop any cached minimum and recompute from current content,
            // otherwise pack() on some platforms won't shrink columns that were previously wider.
            c.setWidth(0);
            c.pack();
            // Add some padding for better readability
            // Column 0 has extra padding because pack() may not count space needed for expand indicator
            c.setWidth(c.getWidth() + ((i == 0) ? 20 : 4));
         }
      }
   }

	/**
	 * Get column object by id (named data with key ID)
	 * @param id Column ID
	 * @return Column object or null if object with given ID not found
	 */
	public TreeColumn getColumnById(int id)
	{
		for(TreeColumn c : columns)
		{
         if ((Integer)c.getData("ID") == id)
			{
				return c;
			}
		}
		return null;
	}

   /**
    * Get column index at given point
    *
    * @param p
    * @return
    */
   public TreeColumn getColumnAtPoint(Point p)
   {
      TreeItem item = getTree().getItem(p);
      if (item == null)
         return null;
      int columnCount = getTree().getColumnCount();
      for(int i = 0; i < columnCount; i++)
      {
         Rectangle rect = item.getBounds(i);
         if (rect.contains(p))
         {
            return getTree().getColumn(i);
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
		for(TreeColumn c : columns)
			c.removeSelectionListener(sortingListener);
		getTree().setSortColumn(null);
	}

   /**
    * Remove column by ID
    *
    * @param id column ID
    */
   public void removeColumnById(int id)
   {
      for(TreeColumn c : columns)
      {
         if (!c.isDisposed() && ((Integer)c.getData("ID") == id))
         {
            columns.remove(c);
            c.dispose();
            return;
         }
      }
   }

   /**
    * Add column to viewer
    *
    * @param name column name
    * @param width column width
    * @return created column object
    */
   public TreeColumn addColumn(String name, int width)
   {
      int index = getTree().getColumnCount();
      TreeColumn c = new TreeColumn(getTree(), SWT.LEFT);
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
      final I18n i18n = LocalizationHelper.getI18n(SortableTreeViewer.class);
      Tree tree = getTree();
      for(TreeColumn c : tree.getColumns())
         c.setMoveable(true);
      tree.setData("persistColumnOrder", Boolean.valueOf(persist));

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

      headerMenu = new Menu(tree);
      headerMenu.addListener(SWT.Show, e -> {
         for(MenuItem item : headerMenu.getItems())
            item.dispose();

         int visibleCount = 0;
         for(TreeColumn c : tree.getColumns())
            if (c.getData("savedWidth") == null)
               visibleCount++;

         if (clickedColumnId >= 0)
         {
            TreeColumn clickedColumn = getColumnById(clickedColumnId);
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

            int[] order = tree.getColumnOrder();
            for(int idx : order)
            {
               TreeColumn c = tree.getColumn(idx);
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

      tree.addListener(SWT.MenuDetect, event -> {
         Point pt = tree.getDisplay().map(null, tree, new Point(event.x, event.y));
         if (tree.getItem(pt) == null && pt.y < tree.getHeaderHeight())
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
      Tree tree = getTree();
      int count = tree.getColumnCount();
      int[] order = new int[count];
      for(int i = 0; i < count; i++)
         order[i] = i;
      tree.setColumnOrder(order);
   }

   /**
    * Hide column by ID (save width, set width to 0, make non-resizable).
    *
    * @param columnId column ID
    */
   public void hideColumn(int columnId)
   {
      TreeColumn column = getColumnById(columnId);
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
      TreeColumn column = getColumnById(columnId);
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
      for(TreeColumn c : getTree().getColumns())
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
      for(TreeColumn c : getTree().getColumns())
         if (c.getData("savedWidth") != null)
            return true;
      return false;
   }

   /**
    * Get column ID at the given header point by walking columns in display order.
    *
    * @param pt point relative to the tree
    * @return column ID or -1 if not found
    */
   private int getColumnIdAtHeaderPoint(Point pt)
   {
      Tree tree = getTree();
      int[] order = tree.getColumnOrder();
      int scrollOffset = 0;
      ScrollBar hBar = tree.getHorizontalBar();
      if (hBar != null)
         scrollOffset = hBar.getSelection();
      int x = scrollOffset;
      for(int idx : order)
      {
         TreeColumn c = tree.getColumn(idx);
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
    * Enable persistence of column settings (widths, order, visibility, sort state) and automatic
    * column resize preference for this viewer. Restores saved values on creation, installs dispose
    * listener that saves them, and enables column reordering.
    *
    * @param configPrefix preference store prefix for persisting column settings
    */
   public void enablePersistence(String configPrefix)
   {
      this.configPrefix = configPrefix;
      if (actionResetColumnOrder == null)
         enableColumnReordering();
      WidgetHelper.restoreTreeViewerSettings(this, configPrefix);
      autoResizeEnabled = PreferenceStore.getInstance().getAsBoolean(configPrefix + ".autoResizeColumns", true);
      getTree().addDisposeListener(e -> {
         WidgetHelper.saveTreeViewerSettings(this, configPrefix);
         PreferenceStore.getInstance().set(configPrefix + ".autoResizeColumns", autoResizeEnabled);
      });
   }

   /**
    * Check if automatic column resize is currently enabled on this viewer.
    *
    * @return true if automatic column resize is enabled
    */
   public boolean isAutoResizeEnabled()
   {
      return autoResizeEnabled;
   }

   /**
    * Set automatic column resize state. Updates toggle action if it has been created. When enabling,
    * columns are packed immediately.
    *
    * @param enabled new state
    */
   public void setAutoResizeEnabled(boolean enabled)
   {
      autoResizeEnabled = enabled;
      if (actionAutoSizeColumns != null)
         actionAutoSizeColumns.setChecked(enabled);
      if (enabled)
         packColumns(true);
   }

   /**
    * Get action for toggling automatic column resize. Returns null if this viewer was not constructed
    * with a configuration prefix (auto-resize state is then fixed and no persistence is available).
    *
    * @return check-box action for toggling automatic column resize, or null
    */
   public Action getAutoSizeColumnsAction()
   {
      if (configPrefix == null)
         return null;
      if (actionAutoSizeColumns == null)
      {
         final I18n i18n = LocalizationHelper.getI18n(SortableTreeViewer.class);
         actionAutoSizeColumns = new Action(i18n.tr("Resize columns automatically"), IAction.AS_CHECK_BOX) {
            @Override
            public void run()
            {
               autoResizeEnabled = isChecked();
               if (autoResizeEnabled)
                  packColumns(true);
            }
         };
         actionAutoSizeColumns.setChecked(autoResizeEnabled);
      }
      return actionAutoSizeColumns;
   }
}
