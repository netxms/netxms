/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 * @author Victor
 *
 */
public class AccessListComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		ITableLabelProvider lp = (ITableLabelProvider)((SortableTableViewer)viewer).getLabelProvider();
		int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");
		String text1 = lp.getColumnText(e1, column);
		String text2 = lp.getColumnText(e2, column);
		if (text1 == null)
			text1 = "";
		if (text2 == null)
			text2 = "";
		int result = text1.compareToIgnoreCase(text2);
		return (((SortableTableViewer) viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
