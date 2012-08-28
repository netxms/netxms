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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.zest.core.viewers.IGraphEntityRelationshipContentProvider;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;

/**
 * Content provider for map
 */
public class MapContentProvider implements IGraphEntityRelationshipContentProvider
{
	private NetworkMapPage page;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IStructuredContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (!(inputElement instanceof NetworkMapPage))
			return null;

		List<NetworkMapElement> elements = new ArrayList<NetworkMapElement>(((NetworkMapPage)inputElement).getElements().size());
		for(NetworkMapElement e : ((NetworkMapPage)inputElement).getElements())
			if (!(e instanceof NetworkMapDecoration))
				elements.add(e);
		return elements.toArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.IGraphEntityRelationshipContentProvider#getRelationships(java.lang.Object, java.lang.Object)
	 */
	@Override
	public Object[] getRelationships(Object source, Object dest)
	{
		NetworkMapLink link = page.findLink((NetworkMapElement)source, (NetworkMapElement)dest);
		if (link != null)
			return new Object[] { link };		
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		if (newInput instanceof NetworkMapPage)
			page = (NetworkMapPage)newInput;
		else
			page = null;
	}

	/**
	 * Get map decorations
	 * 
	 * @param inputElement
	 * @return
	 */
	public List<NetworkMapDecoration> getDecorations(Object inputElement)
	{
		if (!(inputElement instanceof NetworkMapPage))
			return null;

		List<NetworkMapDecoration> elements = new ArrayList<NetworkMapDecoration>(((NetworkMapPage)inputElement).getElements().size());
		for(NetworkMapElement e : ((NetworkMapPage)inputElement).getElements())
			if (e instanceof NetworkMapDecoration)
				elements.add((NetworkMapDecoration)e);
		return elements;
	}
}
