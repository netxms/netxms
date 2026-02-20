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
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeColumn;
import org.eclipse.swt.widgets.TreeItem;
import org.netxms.nxmc.base.widgets.helpers.TreeSortingListener;
import org.netxms.nxmc.localization.LocalizationHelper;

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

	/**
	 * Constructor
	 *
	 * @param parent Parent composite for table control
	 * @param names Column names
	 * @param widths Column widths (may be null)
	 * @param defaultSortingColumn Index of default sorting column
	 */
	public SortableTreeViewer(Composite parent, String[] names, int[] widths,
	                           int defaultSortingColumn, int defaultSortDir,
	                           int style)
	{
		super(new Tree(parent, (style == DEFAULT_STYLE) ? (SWT.MULTI | SWT.FULL_SELECTION) : style));
		getTree().setLinesVisible(true);
		getTree().setHeaderVisible(true);
		createColumns(names, widths, defaultSortingColumn, defaultSortDir);
	}

	/**
	 * Constructor for delayed initialization
	 *
	 * @param parent
	 * @param style
	 */
	public SortableTreeViewer(Composite parent, int style)
	{
		super(new Tree(parent, (style == DEFAULT_STYLE) ? (SWT.MULTI | SWT.FULL_SELECTION) : style));
		getTree().setLinesVisible(true);
		getTree().setHeaderVisible(true);
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

		sortingListener = new TreeSortingListener(this);

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
    * Pack columns
    */
   public void packColumns()
   {
      Tree tree = getTree();
      int count = tree.getColumnCount();
      for(int i = 0; i < count; i++)
         tree.getColumn(i).pack();
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
			if ((Integer)c.getData("ID") == id) //$NON-NLS-1$
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
         if (!c.isDisposed() && ((Integer)c.getData("ID") == id)) //$NON-NLS-1$
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
      c.setData("ID", Integer.valueOf(index)); //$NON-NLS-1$
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
      Tree tree = getTree();
      for(TreeColumn c : tree.getColumns())
         c.setMoveable(true);
      tree.setData("persistColumnOrder", Boolean.valueOf(persist));

      String menuText = LocalizationHelper.getI18n(SortableTreeViewer.class).tr("Restore Default Column Order");

      actionResetColumnOrder = new Action(menuText) {
         @Override
         public void run()
         {
            resetColumnOrder();
         }
      };

      Menu headerMenu = new Menu(tree);
      MenuItem resetItem = new MenuItem(headerMenu, SWT.PUSH);
      resetItem.setText(menuText);
      resetItem.addListener(SWT.Selection, e -> resetColumnOrder());

      tree.addListener(SWT.MenuDetect, event -> {
         Point pt = tree.getDisplay().map(null, tree, new Point(event.x, event.y));
         if (pt.y < tree.getHeaderHeight())
         {
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
    * Get action for resetting column order to default. Returns null if column reordering is not enabled.
    *
    * @return action for resetting column order or null
    */
   public Action getResetColumnOrderAction()
   {
      return actionResetColumnOrder;
   }
}
