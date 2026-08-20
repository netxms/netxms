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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.TreeMap;
import org.eclipse.core.runtime.Assert;
import org.eclipse.jface.viewers.CellNavigationStrategy;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.accessibility.ACC;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.Widget;
import org.netxms.nxmc.base.widgets.SortableTableViewer;

/**
 * Cell selection manager. Replaces native row selection in table viewer with selection of individual cells. Supports single cell
 * selection, selection of individual cells with Ctrl key, and range selection with Shift key.
 */
public class CellSelectionManager
{
   private SortableTableViewer viewer;
   private CellNavigationStrategy navigationStrategy;
   private CellSelectionHighlighter cellHighlighter;
   private ViewerCell focusCell;
   private ViewerCell anchorCell;
   private Set<ViewerCell> selectedCells = new HashSet<ViewerCell>();
   private boolean rangeSelection = false;
   private DisposeListener itemDeletionListener = (e) -> clearSelection();

   /**
    * Create cell selection manager for given viewer.
    *
    * @param viewer table viewer
    */
   public CellSelectionManager(SortableTableViewer viewer)
   {
      this.viewer = viewer;
      cellHighlighter = new CellSelectionHighlighter(viewer, this);
      navigationStrategy = new CellNavigationStrategy();
      hookListener(viewer);
   }

   /**
    * Handle mouse button press.
    *
    * @param event mouse event
    */
   private void handleMouseDown(Event event)
   {
      ViewerCell cell = viewer.getCell(new Point(event.x, event.y));
      if (cell == null)
         return;

      if (event.button == 3)
      {
         // Right click within existing selection should not change it (context menu will be shown for all selected cells)
         if (selectedCells.contains(cell))
            setFocusCell(cell);
         else
            selectSingleCell(cell);
      }
      else if ((event.stateMask & SWT.SHIFT) != 0)
      {
         selectRange(cell, (event.stateMask & SWT.CTRL) != 0);
      }
      else if ((event.stateMask & SWT.CTRL) != 0)
      {
         toggleCell(cell);
      }
      else
      {
         selectSingleCell(cell);
      }
   }

   /**
    * Handle key press.
    *
    * @param event key event
    */
   private void handleKeyDown(Event event)
   {
      if (navigationStrategy.isCollapseEvent(viewer, focusCell, event))
      {
         navigationStrategy.collapse(viewer, focusCell, event);
      }
      else if (navigationStrategy.isExpandEvent(viewer, focusCell, event))
      {
         navigationStrategy.expand(viewer, focusCell, event);
      }
      else if (navigationStrategy.isNavigationEvent(viewer, event))
      {
         ViewerCell cell = navigationStrategy.findSelectedCell(viewer, focusCell, event);
         if ((cell != null) && !cell.equals(focusCell))
         {
            if ((event.stateMask & SWT.SHIFT) != 0)
               selectRange(cell, false);
            else
               selectSingleCell(cell);
         }
      }

      if (navigationStrategy.shouldCancelEvent(viewer, event))
      {
         event.doit = false;
      }
   }

   /**
    * Handle selection change in underlying table. Native selection is only used as a fallback for selection changes not initiated
    * by this manager (for example, selection changed by the widget itself).
    *
    * @param event selection event
    */
   private void handleSelection(Event event)
   {
      if (rangeSelection)
         return; // Native selection changes caused by Shift+click / Shift+arrow should not reset cell selection

      if (((event.detail & SWT.CHECK) == 0) && (focusCell != null) && (focusCell.getItem() != event.item) && (event.item != null) &&
            !event.item.isDisposed())
      {
         ViewerRow row = viewer.getViewerRowFromItem(event.item);
         Assert.isNotNull(row, "Internal Structure invalid. Row item has no row ViewerRow assigned");
         ViewerCell cell = row.getCell(focusCell.getColumnIndex());
         if (!focusCell.equals(cell))
         {
            selectSingleCell(cell);
         }
      }
   }

   /**
    * Handle the {@link SWT#FocusIn} event.
    *
    * @param event the event
    */
   private void handleFocusIn(Event event)
   {
      if (focusCell == null)
      {
         ViewerCell cell = getInitialFocusCell();
         if (cell != null)
            selectSingleCell(cell);
      }
   }

   /**
    * Get cell that should receive focus when table viewer gets focus for the first time.
    *
    * @return cell to be focused or null
    */
   private ViewerCell getInitialFocusCell()
   {
      Table table = viewer.getTable();

      if (!table.isDisposed() && (table.getItemCount() > 0) && !table.getItem(table.getTopIndex()).isDisposed())
      {
         final ViewerRow row = viewer.getViewerRowFromItem(table.getItem(table.getTopIndex()));
         if (table.getColumnCount() == 0)
         {
            return row.getCell(0);
         }

         Rectangle clientArea = table.getClientArea();
         for(int i = 0; i < table.getColumnCount(); i++)
         {
            if (columnInVisibleArea(clientArea, row, i))
               return row.getCell(i);
         }
      }

      return null;
   }

   /**
    * Check if given column is within visible part of the table.
    *
    * @param clientArea table client area
    * @param row table row
    * @param colIndex column index
    * @return true if column is visible
    */
   private boolean columnInVisibleArea(Rectangle clientArea, ViewerRow row, int colIndex)
   {
      return row.getBounds(colIndex).x >= clientArea.x;
   }

   /**
    * Select single cell, dropping any existing selection. Selected cell becomes both focus cell and anchor cell for subsequent
    * range selection.
    *
    * @param cell cell to select (can be null to clear selection)
    */
   private void selectSingleCell(ViewerCell cell)
   {
      rangeSelection = false;
      Set<ViewerCell> selection = new HashSet<ViewerCell>();
      if (cell != null)
         selection.add(cell);
      updateSelection(selection);
      anchorCell = cell;
      setFocusCell(cell);
   }

   /**
    * Add cell to selection or remove it from selection if it is already selected. Cell becomes both focus cell and anchor cell for
    * subsequent range selection.
    *
    * @param cell cell to toggle
    */
   private void toggleCell(ViewerCell cell)
   {
      rangeSelection = false;
      Set<ViewerCell> selection = new HashSet<ViewerCell>(selectedCells);
      if (!selection.remove(cell))
         selection.add(cell);
      updateSelection(selection);
      anchorCell = cell;
      setFocusCell(cell);
   }

   /**
    * Select rectangular range of cells between anchor cell and given cell. Anchor cell is not changed, so subsequent range
    * selection will start from same anchor.
    *
    * @param cell cell at the opposite corner of the range
    * @param keepExistingSelection true to add range to existing selection instead of replacing it
    */
   private void selectRange(ViewerCell cell, boolean keepExistingSelection)
   {
      ViewerCell anchor = ((anchorCell != null) && !anchorCell.getItem().isDisposed()) ? anchorCell : focusCell;
      if ((anchor == null) || anchor.getItem().isDisposed())
      {
         selectSingleCell(cell);
         return;
      }

      Set<ViewerCell> selection = keepExistingSelection ? new HashSet<ViewerCell>(selectedCells) : new HashSet<ViewerCell>();
      selection.addAll(buildRange(anchor, cell));
      rangeSelection = true;
      updateSelection(selection);
      setFocusCell(cell);
   }

   /**
    * Build list of cells within rectangular range defined by two corner cells. Range is built using visual column order, so cells
    * within range are always displayed as a solid rectangle. Cells in hidden columns are excluded.
    *
    * @param c1 first corner cell
    * @param c2 second corner cell
    * @return cells within range
    */
   private List<ViewerCell> buildRange(ViewerCell c1, ViewerCell c2)
   {
      List<ViewerCell> cells = new ArrayList<ViewerCell>();
      Table table = viewer.getTable();

      int firstRow = table.indexOf((TableItem)c1.getViewerRow().getItem());
      int lastRow = table.indexOf((TableItem)c2.getViewerRow().getItem());
      if ((firstRow == -1) || (lastRow == -1))
         return cells;
      if (firstRow > lastRow)
      {
         int t = firstRow;
         firstRow = lastRow;
         lastRow = t;
      }

      int[] columnOrder = (table.getColumnCount() > 0) ? table.getColumnOrder() : new int[] { 0 };
      int firstColumn = indexOf(columnOrder, c1.getColumnIndex());
      int lastColumn = indexOf(columnOrder, c2.getColumnIndex());
      if ((firstColumn == -1) || (lastColumn == -1))
         return cells;
      if (firstColumn > lastColumn)
      {
         int t = firstColumn;
         firstColumn = lastColumn;
         lastColumn = t;
      }

      for(int i = firstRow; i <= lastRow; i++)
      {
         TableItem item = table.getItem(i);
         if (item.isDisposed())
            continue;
         ViewerRow row = viewer.getViewerRowFromItem(item);
         if (row == null)
            continue;
         for(int j = firstColumn; j <= lastColumn; j++)
         {
            int columnIndex = columnOrder[j];
            if ((table.getColumnCount() > 0) && (table.getColumn(columnIndex).getWidth() == 0))
               continue; // Skip hidden columns
            ViewerCell cell = row.getCell(columnIndex);
            if (cell != null)
               cells.add(cell);
         }
      }

      return cells;
   }

   /**
    * Find position of given element in integer array.
    *
    * @param array array to search in
    * @param element element to find
    * @return element position or -1 if not found
    */
   private static int indexOf(int[] array, int element)
   {
      for(int i = 0; i < array.length; i++)
         if (array[i] == element)
            return i;
      return -1;
   }

   /**
    * Replace current cell selection with given one and notify highlighter about the change.
    *
    * @param selection new cell selection
    */
   private void updateSelection(Set<ViewerCell> selection)
   {
      Set<ViewerCell> added = new HashSet<ViewerCell>(selection);
      added.removeAll(selectedCells);
      Set<ViewerCell> removed = new HashSet<ViewerCell>(selectedCells);
      removed.removeAll(selection);
      if (added.isEmpty() && removed.isEmpty())
         return;

      selectedCells = selection;
      updateRowSelection();
      cellHighlighter.selectionChanged(added, removed);
   }

   /**
    * Synchronize native row selection with cell selection - rows containing at least one selected cell are selected.
    */
   private void updateRowSelection()
   {
      Table table = viewer.getTable();
      Set<Integer> rows = new HashSet<Integer>();
      for(ViewerCell cell : selectedCells)
      {
         int index = rowIndex(table, cell);
         if (index != -1)
            rows.add(index);
      }

      int[] indexes = new int[rows.size()];
      int i = 0;
      for(Integer r : rows)
         indexes[i++] = r;

      table.deselectAll();
      if (indexes.length > 0)
         table.select(indexes);
   }

   /**
    * Get index of table row containing given cell.
    *
    * @param table table control
    * @param cell cell
    * @return row index or -1 if row cannot be found
    */
   private static int rowIndex(Table table, ViewerCell cell)
   {
      Widget item = cell.getViewerRow().getItem();
      if (!(item instanceof TableItem) || item.isDisposed())
         return -1;
      return table.indexOf((TableItem)item);
   }

   /**
    * Clear cell selection. Called when table item currently holding focus cell is disposed (table refresh or viewer disposal).
    */
   private void clearSelection()
   {
      rangeSelection = false;
      anchorCell = null;
      if (viewer.getControl().isDisposed())
      {
         selectedCells.clear();
         focusCell = null;
         return;
      }
      updateSelection(new HashSet<ViewerCell>());
      setFocusCell(null);
   }

   /**
    * Hook required listeners on viewer's control.
    *
    * @param viewer table viewer
    */
   private void hookListener(final SortableTableViewer viewer)
   {
      Listener listener = new Listener() {
         public void handleEvent(Event event)
         {
            switch(event.type)
            {
               case SWT.MouseDown:
                  handleMouseDown(event);
                  break;
               case SWT.KeyDown:
                  handleKeyDown(event);
                  break;
               case SWT.Selection:
                  handleSelection(event);
                  break;
               case SWT.FocusIn:
                  handleFocusIn(event);
                  break;
            }
         }
      };

      viewer.getControl().addListener(SWT.MouseDown, listener);
      viewer.getControl().addListener(SWT.KeyDown, listener);
      viewer.getControl().addListener(SWT.Selection, listener);
      viewer.getControl().addListener(SWT.FocusIn, listener);
   }

   /**
    * Get cell with the focus.
    *
    * @return cell with the focus or null
    */
   public ViewerCell getFocusCell()
   {
      return focusCell;
   }

   /**
    * Set focus cell without changing selection.
    *
    * @param cell new focus cell (can be null)
    */
   private void setFocusCell(ViewerCell cell)
   {
      ViewerCell oldCell = focusCell;

      if ((focusCell != null) && !focusCell.getItem().isDisposed())
      {
         focusCell.getItem().removeDisposeListener(itemDeletionListener);
      }

      focusCell = cell;

      if ((focusCell != null) && !focusCell.getItem().isDisposed())
      {
         focusCell.getItem().addDisposeListener(itemDeletionListener);
      }

      if (focusCell != null)
      {
         focusCell.scrollIntoView();
      }

      cellHighlighter.focusCellChanged(focusCell, oldCell);

      viewer.getControl().getAccessible().setFocus(ACC.CHILDID_SELF);
   }

   /**
    * Check if given cell is selected.
    *
    * @param cell cell to check
    * @return true if cell is selected
    */
   protected boolean isCellSelected(ViewerCell cell)
   {
      return selectedCells.contains(cell);
   }

   /**
    * Get all selected cells ordered by row and then by visual column position.
    *
    * @return selected cells
    */
   public ViewerCell[] getSelectedCells()
   {
      Table table = viewer.getTable();
      int[] columnOrder = (table.getColumnCount() > 0) ? table.getColumnOrder() : new int[] { 0 };

      TreeMap<Long, ViewerCell> cells = new TreeMap<Long, ViewerCell>();
      for(ViewerCell cell : selectedCells)
      {
         int row = rowIndex(table, cell);
         if (row == -1)
            continue;
         cells.put(((long)row << 32) + indexOf(columnOrder, cell.getColumnIndex()), cell);
      }

      return cells.values().toArray(new ViewerCell[cells.size()]);
   }
}
