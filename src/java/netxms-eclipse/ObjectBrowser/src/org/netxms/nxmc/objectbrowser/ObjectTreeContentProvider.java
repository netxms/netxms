/**
 * 
 */
package org.netxms.nxmc.objectbrowser;

import java.util.Iterator;

import org.eclipse.jface.viewers.TreeNodeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.*;

/**
 * @author Victor
 *
 */
public class ObjectTreeContentProvider extends TreeNodeContentProvider
{
	private NXCSession session = null;
	
	ObjectTreeContentProvider()
	{
		super();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getChildren(java.lang.Object)
	 */
	@Override
	public Object[] getChildren(Object parentElement)
	{
		return ((NXCObject)parentElement).getChildsAsArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		return (session != null) ? session.getTopLevelObjects() : new NXCObject[0];
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#getParent(java.lang.Object)
	 */
	@Override
	public Object getParent(Object element)
	{
		if (session == null)
			return null;
		
		Iterator<Long> it = ((NXCObject)element).getParents();
		return it.hasNext() ? session.findObjectById(it.next()) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.TreeNodeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
		return ((NXCObject)element).getNumberOfChilds() > 0;
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
