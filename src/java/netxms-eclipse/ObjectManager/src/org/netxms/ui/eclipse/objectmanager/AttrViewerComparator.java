/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager;

import java.util.Map.Entry;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.ui.eclipse.objectmanager.propertypages.CustomAttributes;

/**
 * @author Victor
 *
 */
public class AttrViewerComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
		if (sortColumn == null)
			return 0;
		
		int rc;
		switch((Integer)sortColumn.getData("ID"))
		{
			case CustomAttributes.COLUMN_NAME:
				rc =  ((Entry<String, String>)e1).getKey().compareToIgnoreCase(((Entry<String, String>)e2).getKey());
				break;
			case CustomAttributes.COLUMN_VALUE:
				rc =  ((Entry<String, String>)e1).getValue().compareToIgnoreCase(((Entry<String, String>)e2).getValue());
				break;
			default:
				rc = 0;
				break;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
