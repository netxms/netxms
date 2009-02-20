/**
 * 
 */
package org.netxms.ui.eclipse.tools;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;

/**
 * @author victor
 *
 */
public class TableSortingListener extends SelectionAdapter
{
	private TableViewer viewer;
	
	
	/**
	 * Constructor
	 */
	public TableSortingListener(TableViewer viewer)
	{
		this.viewer = viewer;
	}
	
	
	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
	 */
	@Override
	public void widgetSelected(SelectionEvent e)
	{
		Table table = viewer.getTable();
		TableColumn sortColumn = table.getSortColumn();
		if (sortColumn == (TableColumn)e.getSource())
		{
			table.setSortDirection(table.getSortDirection() == SWT.UP ? SWT.DOWN : SWT.UP);
		}
		else
		{
			table.setSortColumn((TableColumn)e.getSource());
		}
		viewer.refresh(false);
	}
}
