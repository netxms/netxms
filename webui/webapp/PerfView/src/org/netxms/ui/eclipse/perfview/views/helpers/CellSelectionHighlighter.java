/**
 * 
 */
package org.netxms.ui.eclipse.perfview.views.helpers;

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
			cell.setText("<b><span style='color:rgb(255,201,14)'>" + cell.getText() + "</span></b>");
		}
	}
	
	/**
	 * @param cell
	 */
	protected void unmarkCell(ViewerCell cell)
	{
		if (cell == null)
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
