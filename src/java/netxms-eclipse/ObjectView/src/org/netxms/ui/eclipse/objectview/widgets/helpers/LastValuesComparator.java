/**
 * 
 */
package org.netxms.ui.eclipse.objectview.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.eclipse.objectview.widgets.LastValuesView;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 * @author victor
 *
 */
public class LastValuesComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		
		DciValue v1 = (DciValue)e1;
		DciValue v2 = (DciValue)e2;

		switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"))
		{
			case LastValuesView.COLUMN_ID:
				result = (int)(v1.getId() - v2.getId());
				break;
			case LastValuesView.COLUMN_DESCRIPTION:
				result = v1.getDescription().compareToIgnoreCase(v2.getDescription());
				break;
			case LastValuesView.COLUMN_VALUE:
				result = v1.getValue().compareToIgnoreCase(v2.getValue());
				break;
			case LastValuesView.COLUMN_TIMESTAMP:
				result = v1.getTimestamp().compareTo(v2.getTimestamp());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
