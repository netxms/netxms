/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets.helpers;

import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Abstract base class for trace view filters
 */
public abstract class AbstractTraceViewFilter extends ViewerFilter implements AbstractViewerFilter
{
	protected String filterString = null;
   protected long rootObjectId = 0;

	/**
	 * @return the filterString
	 */
	public String getFilterString()
	{
		return filterString;
	}

	/**
    * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
    */
   @Override
	public void setFilterString(String filterString)
	{
		this.filterString = filterString.toLowerCase();
	}

   /**
    * Set root object ID (0 to disable filtering)
    *
    * @param rootObjectId new root object ID
    */
   public void setRootObject(long rootObjectId)
   {
      this.rootObjectId = rootObjectId;
   }
}
