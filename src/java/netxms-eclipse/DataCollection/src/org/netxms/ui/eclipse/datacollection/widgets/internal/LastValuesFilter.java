/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.datacollection.DciValue;

/**
 * Filter for last values view
 */
public class LastValuesFilter extends ViewerFilter
{
	private String filterString = null;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
		if ((filterString == null) || (filterString.isEmpty()))
			return true;
		
		final DciValue value = (DciValue)element;
		return value.getDescription().toLowerCase().contains(filterString);
	}

	/**
	 * @return the filterString
	 */
	public String getFilterString()
	{
		return filterString;
	}

	/**
	 * @param filterString the filterString to set
	 */
	public void setFilterString(String filterString)
	{
		this.filterString = filterString.toLowerCase();
	}
}
