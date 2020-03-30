/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.widgets.helpers;

import org.eclipse.jface.viewers.ViewerCell;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Cell selection highlighter
 */
public class CellSelectionHighlighter
{
   private Display display;

	/**
	 * @param viewer
	 */
	public CellSelectionHighlighter(SortableTableViewer viewer, CellSelectionManager manager)
	{
	   display = viewer.getControl().getDisplay();
	}

	/**
	 * @param newCell
	 * @param oldCell
	 */
	protected void focusCellChanged(ViewerCell newCell, ViewerCell oldCell)
	{
	}	

	/**
	 * @param cell
	 */
	protected void markCell(ViewerCell cell)
	{
		if (cell == null)
			return;
		
	   cell.setBackground(display.getSystemColor(SWT.COLOR_LIST_SELECTION));
	   cell.setForeground(display.getSystemColor(SWT.COLOR_LIST_SELECTION_TEXT));
	}

	/**
	 * @param cell
	 */
	protected void unmarkCell(ViewerCell cell)
	{
		if ((cell == null) || cell.getItem().isDisposed())
			return;
		
	   cell.setBackground(null);
	   cell.setForeground(null);
	}	
}
