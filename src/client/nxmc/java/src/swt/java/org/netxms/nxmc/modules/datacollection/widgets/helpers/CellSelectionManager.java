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
import org.eclipse.swt.widgets.TableItem;
import org.netxms.nxmc.base.widgets.SortableTableViewer;

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
			else if ((event.stateMask & SWT.CTRL) != 0)
			{
            setFocusCell(cell, false);
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
			Assert.isNotNull(row, "Internal Structure invalid. Row item has no row ViewerRow assigned"); 
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
					setFocusCell(null, true);
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
		{
			selectedCells.clear();
			viewer.getTable().deselectAll();
			if (focusCell != null)
			{
            selectedCells.add(focusCell);
            viewer.getTable().select(viewer.getTable().indexOf((TableItem)focusCell.getViewerRow().getItem()));
			}
		}
		else if (focusCell != null)
		{
		   if (selectedCells.contains(focusCell))
		   {
		      selectedCells.remove(focusCell);
		      ViewerRow row = focusCell.getViewerRow();
		      boolean removeRowSelection = true;
		      for(ViewerCell c : selectedCells)
		      {
		         if (row.equals(c.getViewerRow()))
		         {
		            // other cells selected in same row
		            removeRowSelection = false;
		            break;
		         }
		      }
		      if (removeRowSelection)
		      {
		         viewer.getTable().deselect(viewer.getTable().indexOf((TableItem)row.getItem()));
		      }
		      else
		      {
		         // keep selection
               viewer.getTable().select(viewer.getTable().indexOf((TableItem)row.getItem()));
		      }
		   }
		   else
		   {
		      selectedCells.add(focusCell);
	         viewer.getTable().select(viewer.getTable().indexOf((TableItem)focusCell.getViewerRow().getItem()));
		   }
		}
		
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

		cellHighlighter.focusCellChanged(focusCell, oldCell);

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
