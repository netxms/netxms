/**
 * 
 */
package org.netxms.ui.eclipse.objecttools.views.helpers;

import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.Table;

/**
 * Content provider for NetXMS table viewer
 *
 */
public class TableContentProvider implements IStructuredContentProvider
{
	private Table table = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		if ((newInput != null) && (newInput instanceof Table))
		{
			table = (Table)newInput;
		}
		else
		{
			table = null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (table == null)
			return new Object[0];
		
		return table.getAllRows();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}
}
