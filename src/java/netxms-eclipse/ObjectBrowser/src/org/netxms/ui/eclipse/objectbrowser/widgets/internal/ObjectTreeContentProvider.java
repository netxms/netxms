/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser.widgets.internal;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.viewers.TreeNodeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;

/**
 * @author Victor
 *
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
