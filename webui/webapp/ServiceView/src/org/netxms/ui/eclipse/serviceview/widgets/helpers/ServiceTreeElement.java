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
package org.netxms.ui.eclipse.serviceview.widgets.helpers;

import java.util.HashSet;
import java.util.Set;
import org.netxms.client.objects.AbstractObject;

/**
 * Service tree element
 *
 */
public class ServiceTreeElement
{
	private AbstractObject object;
	private boolean expanded;
	private ServiceTreeElement parent;
	private Set<ServiceTreeElement> childs = new HashSet<ServiceTreeElement>();
	
	protected ServiceTreeElement(ServiceTreeElement parent, AbstractObject object)
	{
		this.parent = parent;
		this.object = object;
		if (parent != null)
			parent.addChild(this);
	}
	
	/**
	 * Add new child element
	 * 
	 * @param e child element
	 */
	private void addChild(ServiceTreeElement e)
	{
		childs.add(e);
	}
	
	/**
	 * Get element's connections
	 * @return element's connections
	 */
	public ServiceTreeElement[] getConnections()
	{
		return expanded ? childs.toArray(new ServiceTreeElement[childs.size()]) : new ServiceTreeElement[0];
	}

	/**
	 * @return the expanded
	 */
	public boolean isExpanded()
	{
		return expanded;
	}

	/**
	 * @param expanded the expanded to set
	 */
	public void setExpanded(boolean expanded)
	{
		this.expanded = expanded;
		if (!expanded)
		{
			for(ServiceTreeElement e : childs)
				e.setExpanded(false);
		}
	}

	/**
	 * @return the object
	 */
	public AbstractObject getObject()
	{
		return object;
	}

	/**
	 * @return the parent
	 */
	public ServiceTreeElement getParent()
	{
		return parent;
	}

	/**
	 * Check if element has children.
	 * 
	 * @return
	 */
	public boolean hasChildren()
	{
		return childs.size() > 0;
	}
}
