/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import org.eclipse.jface.action.Action;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Item;

/**
 * Common interface for viewers with sortable, persistable and resizable columns ({@link SortableTableViewer} and
 * {@link SortableTreeViewer}). Columns are identified by column ID (named data with key "ID"), which is the column index at
 * creation time and does not change when columns are removed or reordered.
 */
public interface SortableViewer
{
   /**
    * Get viewer's control.
    *
    * @return viewer's control
    */
   public Control getControl();

   /**
    * Create columns. Does nothing if columns were already created.
    *
    * @param names column names
    * @param widths column widths (may be null)
    * @param defaultSortingColumn index of default sorting column
    * @param defaultSortDir default sorting direction
    */
   public void createColumns(String[] names, int[] widths, int defaultSortingColumn, int defaultSortDir);

   /**
    * Add column to viewer.
    *
    * @param name column name
    * @param width column width
    * @return created column object
    */
   public Item addColumn(String name, int width);

   /**
    * Get column object by ID.
    *
    * @param id column ID
    * @return column object or null if column with given ID not found
    */
   public Item getColumnById(int id);

   /**
    * Get ID of column at given index.
    *
    * @param index column index in the underlying control
    * @return column ID
    */
   public int getColumnId(int index);

   /**
    * Remove column by ID.
    *
    * @param id column ID
    */
   public void removeColumnById(int id);

   /**
    * Get column of the item cell at given point.
    *
    * @param p point in control coordinates
    * @return column at given point or null if there is no item at given point
    */
   public Item getColumnAtPoint(Point p);

   /**
    * Check if columns are created.
    *
    * @return true if columns are created
    */
   public boolean isInitialized();

   /**
    * Remove all columns and reset viewer to uninitialized state.
    */
   public void reset();

   /**
    * Disable sorting.
    */
   public void disableSorting();

   /**
    * Get ID of current sort column.
    *
    * @return ID of current sort column or -1 if viewer is not sorted
    */
   public int getSortColumnId();

   /**
    * Get current sort direction.
    *
    * @return current sort direction (SWT.UP, SWT.DOWN, or SWT.NONE)
    */
   public int getSortDirection();

   /**
    * Set sort column and direction. Viewer is not refreshed.
    *
    * @param columnId ID of new sort column
    * @param direction new sort direction (SWT.UP, SWT.DOWN, or SWT.NONE)
    */
   public void setSortColumn(int columnId, int direction);

   /**
    * Pack columns unconditionally (equivalent to <code>packColumns(true)</code>).
    */
   public void packColumns();

   /**
    * Pack columns. When <code>force</code> is <code>false</code>, columns are packed only if automatic column resize is enabled
    * and content has grown past the widths of the last pack. When <code>force</code> is <code>true</code>, columns are packed
    * unconditionally.
    *
    * @param force true to pack columns regardless of automatic resize setting and current content
    */
   public void packColumns(boolean force);

   /**
    * Reset baseline used by automatic column resize, so next pack can shrink columns.
    */
   public void resetAutoResizeBaseline();

   /**
    * Enable column reordering with persistence.
    */
   public void enableColumnReordering();

   /**
    * Enable column reordering.
    *
    * @param persist true to persist column order
    */
   public void enableColumnReordering(boolean persist);

   /**
    * Reset column order to default.
    */
   public void resetColumnOrder();

   /**
    * Hide column.
    *
    * @param columnId column ID
    */
   public void hideColumn(int columnId);

   /**
    * Show previously hidden column.
    *
    * @param columnId column ID
    */
   public void showColumn(int columnId);

   /**
    * Show all hidden columns.
    */
   public void showAllColumns();

   /**
    * Check if viewer has hidden columns.
    *
    * @return true if viewer has hidden columns
    */
   public boolean hasHiddenColumns();

   /**
    * Get "reset column order" action.
    *
    * @return "reset column order" action or null if column reordering is not enabled
    */
   public Action getResetColumnOrderAction();

   /**
    * Get "show all columns" action.
    *
    * @return "show all columns" action or null if column reordering is not enabled
    */
   public Action getShowAllColumnsAction();

   /**
    * Get "auto size columns" action.
    *
    * @return "auto size columns" action or null if persistence is not enabled
    */
   public Action getAutoSizeColumnsAction();

   /**
    * Enable persistence of column settings (widths, order, visibility, sorting, and automatic resize flag).
    *
    * @param configPrefix preference store prefix for persisting column settings
    */
   public void enablePersistence(String configPrefix);

   /**
    * Check if automatic column resize is enabled.
    *
    * @return true if automatic column resize is enabled
    */
   public boolean isAutoResizeEnabled();

   /**
    * Enable or disable automatic column resize. When enabling, columns are packed immediately.
    *
    * @param enabled true to enable automatic column resize
    */
   public void setAutoResizeEnabled(boolean enabled);
}
