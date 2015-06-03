/**
 * 
 */
package org.netxms.ui.eclipse.filemanager.views.helpers;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.server.ServerFile;
import org.netxms.ui.eclipse.filemanager.views.ServerFileManager;

/**
 * Comparator for ServerFile objects
 *
 */
public class ServerFileComparator extends ViewerComparator
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
		switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
		{
			case ServerFileManager.COLUMN_NAME:
				rc = ((ServerFile)e1).getName().compareToIgnoreCase(((ServerFile)e2).getName());
				break;
			case ServerFileManager.COLUMN_TYPE:
            rc = ((ServerFile)e1).getExtension().compareToIgnoreCase(((ServerFile)e2).getExtension());
				break;
			case ServerFileManager.COLUMN_SIZE:
				rc = Long.signum(((ServerFile)e1).getSize() - ((ServerFile)e2).getSize());
				break;
			case ServerFileManager.COLUMN_MODIFYED:
				rc = ((ServerFile)e1).getModifyicationTime().compareTo(((ServerFile)e2).getModifyicationTime());
				break;
			default:
				rc = 0;
				break;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
