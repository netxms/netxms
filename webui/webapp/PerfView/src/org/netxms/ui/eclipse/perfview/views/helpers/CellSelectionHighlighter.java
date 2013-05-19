/**
 * 
 */
package org.netxms.ui.eclipse.perfview.views.helpers;

import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

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
		/*
		viewer.getControl().addListener(SWT.PaintItem, new Listener() {
			public void handleEvent(Event event)
			{
				ViewerCell focusCell = getFocusCell();
				ViewerRow row = CellSelectionHighlighter.this.viewer.getViewerRowFromItem(event.item);
				ViewerCell cell = row.getCell(event.index);

				if (cell.equals(focusCell))
					markFocusedCell(event, cell);
				
				event.detail &= ~SWT.FOCUSED;
			}
		});
		
		viewer.getControl().addListener(SWT.EraseItem, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				ViewerRow row = CellSelectionHighlighter.this.viewer.getViewerRowFromItem(event.item);
				ViewerCell cell = row.getCell(event.index);

				if (CellSelectionHighlighter.this.manager.isCellSelected(cell))
					markSelectedCell(event, cell);

				event.detail &= ~(SWT.SELECTED | SWT.FOCUSED | SWT.HOT);
			}
		});

		viewer.getControl().addListener(SWT.MeasureItem, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
			}
		});
		*/
	}
	
	/**
	 * @param event
	 * @param cell
	 */
	private void markSelectedCell(Event event, ViewerCell cell)
	{
		GC gc = event.gc;
		Display display = cell.getControl().getDisplay();
		gc.setBackground(display.getSystemColor(SWT.COLOR_LIST_SELECTION));
		gc.setForeground(display.getSystemColor(SWT.COLOR_LIST_SELECTION_TEXT));
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
		if (newCell != null)
		{
			Rectangle rect = newCell.getBounds();
			int x = newCell.getColumnIndex() == 0 ? 0 : rect.x;
			int width = newCell.getColumnIndex() == 0 ? rect.x + rect.width : rect.width;
			// 1 is a fix for Linux-GTK
			newCell.getControl().redraw(x, rect.y - 1, width, rect.height + 1, true);
		}

		if (oldCell != null)
		{
			Rectangle rect = oldCell.getBounds();
			int x = oldCell.getColumnIndex() == 0 ? 0 : rect.x;
			int width = oldCell.getColumnIndex() == 0 ? rect.x + rect.width : rect.width;
			// 1 is a fix for Linux-GTK
			oldCell.getControl().redraw(x, rect.y - 1, width, rect.height + 1, true);
		}
	}	
}
