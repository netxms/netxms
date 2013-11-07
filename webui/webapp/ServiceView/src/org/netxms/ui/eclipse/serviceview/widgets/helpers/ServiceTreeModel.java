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

import java.util.ArrayList;
import java.util.List;

import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;

/**
 * Service tree model
 *
 */
public class ServiceTreeModel
{
	private AbstractObject root;
	private List<ServiceTreeElement> elements = new ArrayList<ServiceTreeElement>();
	
	public ServiceTreeModel(AbstractObject root)
	{
		this.root = root;
		addToModel(null, root, 0);
	}
	
	/**
	 * Add given object and it's appropriate child objects to model
	 * 
	 * @param object NetXMS object
	 */
	private void addToModel(ServiceTreeElement parent, AbstractObject object, int level)
	{
		final ServiceTreeElement element = new ServiceTreeElement(parent, object);
		elements.add(element);
		for(AbstractObject o : object.getChildsAsArray())
		{
			if ((o instanceof Container) || (o instanceof Node) || (o instanceof Cluster) || (o instanceof Condition))
			{
				addToModel(element, o, level + 1);
			}
		}
	}
	
	/**
	 * get all elements
	 * 
	 * @return all elements in service tree model
	 */
	public ServiceTreeElement[] getVisibleElements()
	{
		List<ServiceTreeElement> list = new ArrayList<ServiceTreeElement>(elements.size());
		for(ServiceTreeElement e : elements)
		{
			if ((e.getParent() == null) || (e.getParent().isExpanded()))
				list.add(e);
		}
		return list.toArray(new ServiceTreeElement[list.size()]);
	}

	/**
	 * @return the root
	 */
	public AbstractObject getRoot()
	{
		return root;
	}
}
