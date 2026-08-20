/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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

import java.util.Set;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.netxms.nxmc.base.widgets.SortableTableViewer;

/**
 * Cell selection highlighter
 */
public class CellSelectionHighlighter
{
	private SortableTableViewer viewer;
	private CellSelectionManager manager;
	
	/**
	 * @param viewer
	 */
	public CellSelectionHighlighter(SortableTableViewer viewer, CellSelectionManager manager)
	{
		this.viewer = viewer;
		this.manager = manager;

      // Disable native table selection completely
      viewer.getTable().addListener(SWT.Selection, (e) -> viewer.getTable().deselectAll());

      viewer.getControl().addListener(SWT.PaintItem, (event) -> {
         ViewerCell focusCell = getFocusCell();
         ViewerRow row = CellSelectionHighlighter.this.viewer.getViewerRowFromItem(event.item);
         ViewerCell cell = row.getCell(event.index);

         if (cell.equals(focusCell))
            markFocusedCell(event, cell);
         event.gc.setForeground(cell.getControl().getDisplay().getSystemColor(CellSelectionHighlighter.this.manager.isCellSelected(cell) ? SWT.COLOR_LIST_SELECTION_TEXT : SWT.COLOR_LIST_FOREGROUND));

         event.detail &= ~SWT.FOCUSED;
		});

      viewer.getControl().addListener(SWT.EraseItem, (event) -> {
         ViewerRow row = CellSelectionHighlighter.this.viewer.getViewerRowFromItem(event.item);
         ViewerCell cell = row.getCell(event.index);
         drawCellBackground(event, cell, CellSelectionHighlighter.this.manager.isCellSelected(cell));
         event.detail &= ~(SWT.SELECTED | SWT.FOCUSED | SWT.HOT | SWT.BACKGROUND);
		});

      viewer.getControl().addListener(SWT.MeasureItem, (e) -> {});
	}

	/**
	 * @param event
	 * @param cell
	 */
	private void drawCellBackground(Event event, ViewerCell cell, boolean selected)
	{
		GC gc = event.gc;
		Display display = cell.getControl().getDisplay();
		gc.setBackground(display.getSystemColor(selected ? SWT.COLOR_LIST_SELECTION : SWT.COLOR_LIST_BACKGROUND));
		gc.setForeground(display.getSystemColor(selected ? SWT.COLOR_LIST_SELECTION_TEXT : SWT.COLOR_LIST_FOREGROUND));
		gc.fillRectangle(cell.getBounds());
	}

	/**
	 * @param event
	 * @param cell
	 */
	private void markFocusedCell(Event event, ViewerCell cell)
	{
		GC gc = event.gc;
		Rectangle r = cell.getBounds();
		gc.drawFocus(r.x, r.y, r.width, r.height);		
	}

	/**
	 * @return the focus cell
	 */
	private ViewerCell getFocusCell()
	{
		if (manager != null)
			return manager.getFocusCell();
		return viewer.getColumnViewerEditor().getFocusCell();
	}

	/**
	 * @param newCell
	 * @param oldCell
	 */
	protected void focusCellChanged(ViewerCell newCell, ViewerCell oldCell)
	{
		// Redraw new area
      redrawCell(newCell);
      redrawCell(oldCell);
	}

   /**
    * Called by selection manager when set of selected cells is changed.
    *
    * @param added cells added to selection
    * @param removed cells removed from selection
    */
   protected void selectionChanged(Set<ViewerCell> added, Set<ViewerCell> removed)
   {
      if (!viewer.getTable().isDisposed())
         viewer.getTable().redraw();
   }

   /**
    * Redraw area occupied by given cell.
    *
    * @param cell cell to redraw (can be null)
    */
   private static void redrawCell(ViewerCell cell)
   {
      if ((cell == null) || cell.getItem().isDisposed() || cell.getControl().isDisposed())
         return;

      Rectangle rect = cell.getBounds();
      int x = cell.getColumnIndex() == 0 ? 0 : rect.x;
      int width = cell.getColumnIndex() == 0 ? rect.x + rect.width : rect.width;
      // 1 is a fix for Linux-GTK
      cell.getControl().redraw(x, rect.y - 1, width, rect.height + 1, true);
   }
}
