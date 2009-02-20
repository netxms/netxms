/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCObject;

/**
 * @author Victor
 *
 */
public class ObjectListFilter extends ViewerFilter
{
	private String filterString = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
		if (filterString == null)
			return true;
		
		return ((NXCObject)element).getObjectName().toLowerCase().startsWith(filterString);
	}
	
	
	/**
	 * Set filter string
	 */
	public void setFilterString(final String filterString)
	{
		this.filterString = filterString.isEmpty() ? null : filterString.toLowerCase();
	}
}
