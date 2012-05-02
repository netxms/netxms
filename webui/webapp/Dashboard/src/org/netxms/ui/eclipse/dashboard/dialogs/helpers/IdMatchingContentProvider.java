/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.dialogs.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;

/**
 * Content provider for ID matching dialog
 */
public class IdMatchingContentProvider implements ITreeContentProvider
{
	private static final long serialVersionUID = 1L;

	private Map<Long, ObjectIdMatchingData> objects = null;
	Map<Long, DciIdMatchingData> dcis = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getElements(java.lang.Object)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public Object[] getElements(Object inputElement)
	{
		if (inputElement == null)
			return new Object[0];
		
		Object[] input = (Object[])inputElement;
		Map<Long, ObjectIdMatchingData> o = (Map<Long, ObjectIdMatchingData>)input[0];
		return o.values().toArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
	 */
	@Override
	public Object[] getChildren(Object parentElement)
	{
		if (parentElement instanceof ObjectIdMatchingData)
			return ((ObjectIdMatchingData)parentElement).dcis.toArray();
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
	 */
	@Override
	public Object getParent(Object element)
	{
		if (element instanceof DciIdMatchingData)
			return objects.get(((DciIdMatchingData)element).srcNodeId);
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
		if (element instanceof ObjectIdMatchingData)
			return ((ObjectIdMatchingData)element).dcis.size() > 0;
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#inputChanged(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public void inputChanged(Viewer viewer, Object oldInput, Object newInput)
	{
		if (newInput != null)
		{
			Object[] input = (Object[])newInput;
			objects = (Map<Long, ObjectIdMatchingData>)input[0];
			dcis = (Map<Long, DciIdMatchingData>)input[1];
		}
		else
		{
			objects = new HashMap<Long, ObjectIdMatchingData>(0);
			dcis = new HashMap<Long, DciIdMatchingData>(0);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IContentProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}
}
