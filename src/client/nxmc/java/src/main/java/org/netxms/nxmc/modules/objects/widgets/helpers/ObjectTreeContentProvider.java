/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import java.util.Iterator;
import java.util.Set;
import org.eclipse.jface.viewers.TreeNodeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectFilter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Circuit;
import org.netxms.client.objects.LoadingObject;
import org.netxms.client.objects.Node;

/**
 * Content provider for object tree.
 */
public class ObjectTreeContentProvider extends TreeNodeContentProvider
{
	private NXCSession session = null;
   private Set<Integer> classFilter;
   private ObjectFilter objectFilter;
	private boolean objectFullSync = false;

	/**
    * Create content provider for object tree. If object filter is provided, as top level objects will be selected objects that
    * passed filter themselves and does not have accessible parents at all or none of their parents passed filter.
    *
    * @param objectFullSync true if all objects are synchronized
    * @param classFilter class filter for objects
    * @param objectFilter additional filter for top level objects
    */
   public ObjectTreeContentProvider(boolean objectFullSync, Set<Integer> classFilter, ObjectFilter objectFilter)
	{
		super();
      this.classFilter = classFilter;
      this.objectFilter = objectFilter;
		this.objectFullSync = objectFullSync;
	}

   /**
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getChildren(java.lang.Object)
	 */
	@Override
	public Object[] getChildren(Object parentElement)
	{
      AbstractObject object = (AbstractObject)parentElement;
      if (!objectFullSync && ((object instanceof Node) || (object instanceof Circuit)) && object.hasChildren() && !object.areChildrenSynchronized())
         return new AbstractObject[] { new LoadingObject(-1, session) };
      return object.getChildrenAsArray();
	}

   /**
    * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getElements(java.lang.Object)
    */
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (session != null)
		{
         if (objectFilter == null)
            return session.getTopLevelObjects(classFilter);
         if (classFilter == null)
            return session.getTopLevelObjects(objectFilter);
         return session.getTopLevelObjects((AbstractObject o) -> classFilter.contains(o.getObjectClass()) && objectFilter.accept(o));
		}
		return new AbstractObject[0];
	}

   /**
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

	/**
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
		return ((element instanceof Node) || (element instanceof Circuit)) ? ((AbstractObject)element).hasChildren() : ((AbstractObject)element).hasAccessibleChildren();
	}

	/**
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		session = (NXCSession)newInput;
	}
}
