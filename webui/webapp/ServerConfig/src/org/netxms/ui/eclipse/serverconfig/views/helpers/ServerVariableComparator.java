package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.api.client.servermanager.ServerVariable;
import org.netxms.ui.eclipse.serverconfig.views.ServerConfigurationEditor;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Comparator for server configuration variables
 */
public class ServerVariableComparator extends ViewerComparator
{
	/**
	 * Compare two booleans and return -1, 0, or 1
	 */
	private int compareBooleans(boolean b1, boolean b2)
	{
		return (!b1 && b2) ? -1 : ((b1 && !b2) ? 1 : 0);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result;
		
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
			case ServerConfigurationEditor.COLUMN_NAME:
				result = ((ServerVariable)e1).getName().compareToIgnoreCase(((ServerVariable)e2).getName());
				break;
			case ServerConfigurationEditor.COLUMN_VALUE:
				result = ((ServerVariable)e1).getValue().compareToIgnoreCase(((ServerVariable)e2).getValue());
				break;
			case ServerConfigurationEditor.COLUMN_NEED_RESTART:
				result = compareBooleans(((ServerVariable)e1).isServerRestartNeeded(), ((ServerVariable)e2).isServerRestartNeeded());
				break;
			default:
				result = 0;
				break;
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}