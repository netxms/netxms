/**
 * 
 */
package org.netxms.ui.eclipse.perfview.widgets.helpers;

import org.eclipse.jface.viewers.ViewerCell;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Cell selection highlighter
 */
public class CellSelectionHighlighter
{
	/**
	 * @param viewer
	 */
	public CellSelectionHighlighter(SortableTableViewer viewer, CellSelectionManager manager)
	{
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
		
		final String key = "CELL#" + cell.getColumnIndex();
		if (cell.getItem().getData(key) == null)
		{
			cell.getItem().setData(key, cell.getText());
			// FIXME: read colors from current theme
			cell.setText("<span style='color:white;background-color: #00589f;'>" + cell.getText() + "</span>");
		}
	}
	
	/**
	 * @param cell
	 */
	protected void unmarkCell(ViewerCell cell)
	{
		if ((cell == null) || cell.getItem().isDisposed())
			return;
		
		final String key = "CELL#" + cell.getColumnIndex();
		Object data = cell.getItem().getData(key);
		if (data != null)
		{
			cell.setText((String)data);
			cell.getItem().setData(key, null);
		}
	}	
}
