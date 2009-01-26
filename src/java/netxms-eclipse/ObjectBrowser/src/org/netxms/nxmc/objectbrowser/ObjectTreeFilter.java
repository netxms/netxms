/**
 * 
 */
package org.netxms.nxmc.objectbrowser;

import java.util.regex.Pattern;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCObject;

/**
 * @author Victor
 *
 */
public class ObjectTreeFilter extends ViewerFilter
{
	private String filterString = null;
	//private Pattern pattern = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
		//if ((filterString == null) || (pattern == null))
		if (filterString == null)
			return true;
		
		boolean pass = ((NXCObject)element).getObjectName().toLowerCase().startsWith(filterString);
		if (!pass)
		{
			if (((NXCObject)element).getNumberOfChilds() > 0)
			{
				pass = true;
			}
		}
		return pass;
		//return pattern.matcher(((NXCObject)element).getObjectName()).matches();
	}
	
	
	/**
	 * Set filter string
	 */
	void setFilterString(final String filterString)
	{
		this.filterString = filterString.toLowerCase();
		//pattern = filterString.isEmpty() ? null : Pattern.compile(filterString, Pattern.CASE_INSENSITIVE);
	}
}
