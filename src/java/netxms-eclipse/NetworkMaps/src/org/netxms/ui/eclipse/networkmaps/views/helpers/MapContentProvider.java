/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.zest.core.viewers.IGraphEntityContentProvider;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapElement;

/**
 * Content provider for map
 * 
 */
public class MapContentProvider implements IGraphEntityContentProvider
{
	private NetworkMapPage page;
	
	/**
	 * Create map content provider object
	 */
	public MapContentProvider()
	{
	}
	
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (!(inputElement instanceof NetworkMapPage))
			return null;
		
		return ((NetworkMapPage)inputElement).getElements().toArray();
	}

	@Override
	public Object[] getConnectedTo(Object entity)
	{
		if (!(entity instanceof NetworkMapElement) || (page == null))
			return null;
		
		return page.getConnectedElements(((NetworkMapElement)entity).getId());
	}

	@Override
	public void dispose()
	{
	}

	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		if (newInput instanceof NetworkMapPage)
			page = (NetworkMapPage)newInput;
		else
			page = null;
	}
}
