/**
 * 
 */
package org.netxms.ui.eclipse.perfview.views.helpers;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.Assert;
import org.eclipse.jface.viewers.CellNavigationStrategy;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.accessibility.ACC;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Table;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Cell selection manager
 */
public class CellSelectionManager
{
	private CellNavigationStrategy navigationStrategy;
	private SortableTableViewer viewer;
	private ViewerCell focusCell;
	private Set<ViewerCell> selectedCells = new HashSet<ViewerCell>();
	private CellSelectionHighlighter cellHighlighter;
	private DisposeListener itemDeletionListener = new DisposeListener() {
		public void widgetDisposed(DisposeEvent e)
		{
			setFocusCell(null, true);
		}
	};

	/**
	 * @param viewer
	 * @param focusDrawingDelegate
	 */
	public CellSelectionManager(SortableTableViewer viewer)
	{
		this.viewer = viewer;
		this.cellHighlighter = new CellSelectionHighlighter(viewer, this);
		this.navigationStrategy = new CellNavigationStrategy();
		hookListener(viewer);
	}

	/**
	 * @param event
	 */
	private void handleMouseDown(Event event)
	{
		ViewerCell cell = viewer.getCell(new Point(event.x, event.y));
		if (cell != null)
		{
			if (!cell.equals(focusCell))
			{
				setFocusCell(cell, (event.stateMask & SWT.CTRL) == 0);
			}
		}
	}

	/**
	 * @param event
	 */
	private void handleKeyDown(Event event)
	{
		ViewerCell tmp = null;

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
			tmp = navigationStrategy.findSelectedCell(viewer, focusCell, event);

			if (tmp != null)
			{
				if (!tmp.equals(focusCell))
				{
					setFocusCell(tmp, true);
				}
			}
		}

		if (navigationStrategy.shouldCancelEvent(viewer, event))
		{
			event.doit = false;
		}
	}

	/**
	 * @param event
	 */
	private void handleSelection(Event event)
	{
		if ((event.detail & SWT.CHECK) == 0 && focusCell != null && focusCell.getItem() != event.item && event.item != null
				&& !event.item.isDisposed())
		{
			ViewerRow row = viewer.getViewerRowFromItem(event.item);
			Assert.isNotNull(row, "Internal Structure invalid. Row item has no row ViewerRow assigned"); //$NON-NLS-1$
			ViewerCell tmp = row.getCell(focusCell.getColumnIndex());
			if (!focusCell.equals(tmp))
			{
				setFocusCell(tmp, true);
			}
		}
	}

	/**
	 * Handles the {@link SWT#FocusIn} event.
	 * 
	 * @param event the event
	 */
	private void handleFocusIn(Event event)
	{
		if (focusCell == null)
		{
			setFocusCell(getInitialFocusCell(), true);
		}
	}

	/**
	 * @return
	 */
	private ViewerCell getInitialFocusCell()
	{
		Table table = viewer.getTable();

		if (!table.isDisposed() && table.getItemCount() > 0 && !table.getItem(table.getTopIndex()).isDisposed())
		{
			final ViewerRow aViewerRow = viewer.getViewerRowFromItem(table.getItem(table.getTopIndex()));
			if (table.getColumnCount() == 0)
			{
				return aViewerRow.getCell(0);
			}

			Rectangle clientArea = table.getClientArea();
			for(int i = 0; i < table.getColumnCount(); i++)
			{
				if (columnInVisibleArea(clientArea, aViewerRow, i))
					return aViewerRow.getCell(i);
			}
		}

		return null;
	}
	
	/**
	 * @param clientArea
	 * @param row
	 * @param colIndex
	 * @return
	 */
	private boolean columnInVisibleArea(Rectangle clientArea, ViewerRow row, int colIndex)
	{
		return row.getBounds(colIndex).x >= clientArea.x;
	}

	/**
	 * @param viewer
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
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			public void selectionChanged(SelectionChangedEvent event)
			{
				if (event.getSelection().isEmpty())
				{
					//setFocusCell(null, true);
				}
			}
		});
		viewer.getControl().addListener(SWT.FocusIn, listener);
	}

	/**
	 * @return the cell with the focus
	 * 
	 */
	public ViewerCell getFocusCell()
	{
		return focusCell;
	}

	/**
	 * @param focusCell
	 */
	void setFocusCell(ViewerCell focusCell, boolean clearSelection)
	{
		if (clearSelection)
			selectedCells.clear();
		selectedCells.add(focusCell);
		
		ViewerCell oldCell = this.focusCell;

		if (this.focusCell != null && !this.focusCell.getItem().isDisposed())
		{
			this.focusCell.getItem().removeDisposeListener(itemDeletionListener);
		}

		this.focusCell = focusCell;

		if (this.focusCell != null && !this.focusCell.getItem().isDisposed())
		{
			this.focusCell.getItem().addDisposeListener(itemDeletionListener);
		}

		if (focusCell != null)
		{
			focusCell.scrollIntoView();
		}

		this.cellHighlighter.focusCellChanged(focusCell, oldCell);

		viewer.getControl().getAccessible().setFocus(ACC.CHILDID_SELF);
	}
	
	/**
	 * @return
	 */
	protected boolean isCellSelected(ViewerCell cell)
	{
		return selectedCells.contains(cell);
	}
	
	/**
	 * @return
	 */
	public ViewerCell[] getSelectedCells()
	{
		return selectedCells.toArray(new ViewerCell[selectedCells.size()]);
	}
}
