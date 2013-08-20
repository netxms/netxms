/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;

/**
 * Filter for last values view
 */
public class LastValuesFilter extends ViewerFilter
{
	private String filterString = null;
	private boolean showDisabled = false;
	private boolean showUnsupported = false;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
		final DciValue value = (DciValue)element;
		
		if (!showUnsupported && (value.getStatus() == DataCollectionObject.NOT_SUPPORTED))
			return false;
		
		if (!showDisabled && (value.getStatus() == DataCollectionObject.DISABLED))
			return false;
		
		if ((filterString == null) || (filterString.isEmpty()))
			return true;
		
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

	/**
	 * @return
	 */
	public boolean isShowDisabled() 
	{
		return showDisabled;
	}

	/**
	 * @param showDisabled
	 */
	public void setShowDisabled(boolean showDisabled) 
	{
		this.showDisabled = showDisabled;
	}

	/**
	 * @return the showUnsupported
	 */
	public boolean isShowUnsupported()
	{
		return showUnsupported;
	}

	/**
	 * @param showUnsupported the showUnsupported to set
	 */
	public void setShowUnsupported(boolean showUnsupported)
	{
		this.showUnsupported = showUnsupported;
	}
}
