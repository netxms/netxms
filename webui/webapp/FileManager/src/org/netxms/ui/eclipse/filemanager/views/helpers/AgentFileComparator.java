/**
 * 
 */
package org.netxms.ui.eclipse.filemanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.client.ServerFile;
import org.netxms.ui.eclipse.filemanager.views.ViewServerFile;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Comparator for ServerFile objects
 *
 */
public class AgentFileComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		TreeColumn sortColumn = ((SortableTreeViewer)viewer).getTree().getSortColumn();
		if (sortColumn == null)
			return 0;
		
		int rc;
		switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
		{
		   case ViewServerFile.COLUMN_NAME:
            if(((ServerFile)e1).isDirectory() == ((ServerFile)e2).isDirectory())
            {
               rc = ((ServerFile)e1).getName().compareToIgnoreCase(((ServerFile)e2).getName());
            }
            else
            {
               rc = ((ServerFile)e1).isDirectory() ? -1 : 1;
            }           
            break;
         case ViewServerFile.COLUMN_TYPE:
            if(((ServerFile)e1).isDirectory() == ((ServerFile)e2).isDirectory())
            {
               rc = ((ServerFile)e1).getExtension().compareToIgnoreCase(((ServerFile)e2).getExtension());
            }
            else
            {
               rc = ((ServerFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         case ViewServerFile.COLUMN_SIZE:
            if((((ServerFile)e1).isDirectory() == ((ServerFile)e2).isDirectory()))
            {
               rc = Long.signum(((ServerFile)e1).getSize() - ((ServerFile)e2).getSize());
            }
            else
            {
               rc = ((ServerFile)e1).isDirectory() ? -1 : 1;
            }
            break;
         case ViewServerFile.COLUMN_MODIFYED:
            if((((ServerFile)e1).isDirectory() == ((ServerFile)e2).isDirectory()))
            {
               rc = ((ServerFile)e1).getModifyicationTime().compareTo(((ServerFile)e2).getModifyicationTime());
            }
            else
            {
               rc = ((ServerFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         default:
            rc = 0;
            break;
		}
		int dir = ((SortableTreeViewer)viewer).getTree().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
