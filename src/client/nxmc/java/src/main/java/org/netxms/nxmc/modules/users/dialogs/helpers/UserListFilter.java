/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
package org.netxms.nxmc.modules.users.dialogs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.modules.users.views.helpers.BaseUserLabelProvider;

/**
 * Filter for user list
 */
public class UserListFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filter = "";
	private final BaseUserLabelProvider baseLabelProvider;

   /**
    * Create label provider.
    */
   public UserListFilter(BaseUserLabelProvider baseLabelProvider)
   {
      this.baseLabelProvider = baseLabelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
      return (filter == null) || filter.isEmpty() || (element instanceof String) || baseLabelProvider.getText(element).toLowerCase().contains(filter);
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
	public void setFilterString(String filter)
	{
		this.filter = filter.toLowerCase();
	}	
}
