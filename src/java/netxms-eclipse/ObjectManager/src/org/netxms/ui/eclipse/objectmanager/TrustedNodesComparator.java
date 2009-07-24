/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCObject;
import org.netxms.ui.eclipse.objectmanager.propertypages.TrustedNodes;

/**
 * @author Victor
 *
 */
public class TrustedNodesComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
		if (sortColumn == null)
			return 0;
		
		int rc;
		if ((Integer)sortColumn.getData("ID") == TrustedNodes.COLUMN_NAME)
		{
			rc =  ((NXCObject)e1).getObjectName().compareToIgnoreCase(((NXCObject)e2).getObjectName());
		}
		else
		{
			rc = 0;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
