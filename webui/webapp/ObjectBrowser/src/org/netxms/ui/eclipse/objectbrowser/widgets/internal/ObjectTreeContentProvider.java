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
package org.netxms.ui.eclipse.objectbrowser.widgets.internal;

import java.util.Iterator;
import org.eclipse.jface.viewers.TreeNodeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;

/**
 * Content provider for object tree.
 */
public class ObjectTreeContentProvider extends TreeNodeContentProvider
{
	private NXCSession session = null;
	private long[] rootObjects;
	
	public ObjectTreeContentProvider(long[] rootObjects)
	{
		super();
		this.rootObjects = rootObjects;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getChildren(java.lang.Object)
	 */
	@Override
	public Object[] getChildren(Object parentElement)
	{
		/*
		final GenericObject[] childsAsArray = ((GenericObject)parentElement).getChildsAsArray();
		List<GenericObject> filteredList = new ArrayList<GenericObject>();
		for(GenericObject genericObject : childsAsArray)
		{
			if (!genericObject.getObjectName().startsWith("@"))
			{
				filteredList.add(genericObject);
			}
		}
		return filteredList.toArray();
		*/
		return ((GenericObject)parentElement).getChildsAsArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (session != null)
		{
			return (rootObjects != null) ? session.findMultipleObjects(rootObjects).toArray() : session.getTopLevelObjects();
		}
		return new GenericObject[0];
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getParent(java.lang.Object)
	 */
	@Override
	public Object getParent(Object element)
	{
		if (session == null)
			return null;
		
		Iterator<Long> it = ((GenericObject)element).getParents();
		return it.hasNext() ? session.findObjectById(it.next()) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
		return ((GenericObject)element).getNumberOfChilds() > 0;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		session = (NXCSession)newInput;
		viewer.refresh();
	}
}
