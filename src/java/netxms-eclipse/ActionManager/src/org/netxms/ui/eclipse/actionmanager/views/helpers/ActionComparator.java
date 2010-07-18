/**
 * 
 */
package org.netxms.ui.eclipse.actionmanager.views.helpers;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.ServerAction;
import org.netxms.ui.eclipse.actionmanager.views.ActionManager;

/**
 * @author victor
 *
 */
public class ActionComparator extends ViewerComparator
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
		switch((Integer)sortColumn.getData("ID"))
		{
			case ActionManager.COLUMN_NAME:
				rc = ((ServerAction)e1).getName().compareToIgnoreCase(((ServerAction)e2).getName());
				break;
			case ActionManager.COLUMN_TYPE:
				rc = ((ServerAction)e1).getType() - ((ServerAction)e2).getType();
				break;
			case ActionManager.COLUMN_RCPT:
				rc = ((ServerAction)e1).getRecipientAddress().compareToIgnoreCase(((ServerAction)e2).getRecipientAddress());
				break;
			case ActionManager.COLUMN_SUBJ:
				rc = ((ServerAction)e1).getEmailSubject().compareToIgnoreCase(((ServerAction)e2).getEmailSubject());
				break;
			case ActionManager.COLUMN_DATA:
				rc = ((ServerAction)e1).getData().compareToIgnoreCase(((ServerAction)e2).getData());
				break;
			default:
				rc = 0;
				break;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
