/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.dialogs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.AgentTable;
import org.netxms.ui.eclipse.datacollection.dialogs.AbstractSelectParamDlg;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for agent tables
 */
public class AgentTableComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		AgentTable p1 = (AgentTable)e1;
		AgentTable p2 = (AgentTable)e2;

		switch((Integer)((SortableTableViewer) viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case AbstractSelectParamDlg.COLUMN_NAME:
				result = p1.getName().compareToIgnoreCase(p2.getName());
				break;
			case AbstractSelectParamDlg.COLUMN_TYPE:
				result = p1.getInstanceColumn().compareToIgnoreCase(p2.getInstanceColumn());
				break;
			case AbstractSelectParamDlg.COLUMN_DESCRIPTION:
				result = p1.getDescription().compareToIgnoreCase(p2.getDescription());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
