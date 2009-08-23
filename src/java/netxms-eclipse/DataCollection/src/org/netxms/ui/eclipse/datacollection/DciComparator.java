/**
 * 
 */
package org.netxms.ui.eclipse.datacollection;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.datacollection.views.DataCollectionEditor;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 * @author Victor
 *
 */
public class DciComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;

		DataCollectionItem dci1 = (DataCollectionItem)e1;
		DataCollectionItem dci2 = (DataCollectionItem)e2;

		switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID"))
		{
			case DataCollectionEditor.COLUMN_DESCRIPTION:
				result = dci1.getDescription().compareToIgnoreCase(dci2.getDescription());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
