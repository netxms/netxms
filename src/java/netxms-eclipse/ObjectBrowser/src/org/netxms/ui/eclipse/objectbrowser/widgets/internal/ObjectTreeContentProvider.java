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
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;

/**
 * Content provider for object tree.
 */
public class ObjectTreeContentProvider extends TreeNodeContentProvider
{
	private NXCSession session = null;
	private long[] rootObjects = null;
	private boolean hideNodeComponents = false;
	
	/**
	 * @param rootObjects
	 * @param hideNodeComponents 
	 */
	public ObjectTreeContentProvider(long[] rootObjects, boolean hideNodeComponents)
	{
		super();
		if (rootObjects != null)
		{
			this.rootObjects = new long[rootObjects.length];
			System.arraycopy(rootObjects, 0, this.rootObjects, 0, rootObjects.length);
		}
		this.hideNodeComponents = hideNodeComponents;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getChildren(java.lang.Object)
	 */
	@Override
	public Object[] getChildren(Object parentElement)
	{
		return ((AbstractObject)parentElement).getChildrenAsArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (session != null)
		{
			return (rootObjects != null) ? session.findMultipleObjects(rootObjects, false).toArray() : session.getTopLevelObjects();
		}
		return new AbstractObject[0];
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getParent(java.lang.Object)
	 */
	@Override
	public Object getParent(Object element)
	{
		if (session == null)
			return null;
		
		Iterator<Long> it = ((AbstractObject)element).getParents();
		return it.hasNext() ? session.findObjectById(it.next()) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
	   if(hideNodeComponents && element instanceof Node)
	         return false;
		return ((AbstractObject)element).hasAccessibleChildren();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		session = (NXCSession)newInput;
	}

	/**
	 * @param rootObjects the rootObjects to set
	 */
	public void setRootObjects(long[] rootObjects)
	{
		if (rootObjects != null)
		{
			this.rootObjects = new long[rootObjects.length];
			System.arraycopy(rootObjects, 0, this.rootObjects, 0, rootObjects.length);
		}
		else
		{
			this.rootObjects = null;
		}
	}
	
	/**
	 * Sets flag is node components should be displayed
	 */
	public void setHideNodeComponents(boolean hide)
	{
	   hideNodeComponents = hide;
	}
}
