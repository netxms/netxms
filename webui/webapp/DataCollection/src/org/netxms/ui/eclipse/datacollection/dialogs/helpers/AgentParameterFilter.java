/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.AgentParameter;

/**
 * Filter for agent parameters list
 */
public class AgentParameterFilter extends ViewerFilter
{
	private static final long serialVersionUID = 1L;

	private String filter = "";
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
		return (filter == null) || filter.isEmpty() || ((AgentParameter)element).getName().toLowerCase().startsWith(filter);
	}

	/**
	 * @return the filter
	 */
	public String getFilter()
	{
		return filter;
	}

	/**
	 * @param filter the filter to set
	 */
	public void setFilter(String filter)
	{
		this.filter = filter.toLowerCase();
	}
}
