/**
 * 
 */
package org.netxms.ui.eclipse.filemanager.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.client.server.AgentFile;
import org.netxms.ui.eclipse.filemanager.views.AgentFileManager;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Comparator for AgentFile objects
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
		   case AgentFileManager.COLUMN_NAME:
            if(((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory())
            {
               rc = ((AgentFile)e1).getName().compareToIgnoreCase(((AgentFile)e2).getName());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }           
            break;
         case AgentFileManager.COLUMN_TYPE:
            if(((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory())
            {
               rc = ((AgentFile)e1).getExtension().compareToIgnoreCase(((AgentFile)e2).getExtension());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         case AgentFileManager.COLUMN_SIZE:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = Long.signum(((AgentFile)e1).getSize() - ((AgentFile)e2).getSize());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }
            break;
         case AgentFileManager.COLUMN_MODIFYED:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = ((AgentFile)e1).getModifyicationTime().compareTo(((AgentFile)e2).getModifyicationTime());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         case AgentFileManager.COLUMN_OWNER:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = ((AgentFile)e1).getOwner().compareTo(((AgentFile)e2).getOwner());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         case AgentFileManager.COLUMN_GROUP:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = ((AgentFile)e1).getGroup().compareTo(((AgentFile)e2).getGroup());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
            }  
            break;
         case AgentFileManager.COLUMN_ACCESS_RIGHTS:
            if((((AgentFile)e1).isDirectory() == ((AgentFile)e2).isDirectory()))
            {
               rc = ((AgentFile)e1).getAccessRights().compareTo(((AgentFile)e2).getAccessRights());
            }
            else
            {
               rc = ((AgentFile)e1).isDirectory() ? -1 : 1;
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
