/**
 * 
 */
package org.netxms.ui.eclipse.objectview.objecttabs.helpers;

import org.eclipse.jface.viewers.ITreeContentProvider;
import org.eclipse.jface.viewers.Viewer;
import org.netxms.client.PhysicalComponent;

/**
 * Content provider for component tree
 */
public class ComponentTreeContentProvider implements ITreeContentProvider
{
	private static final long serialVersionUID = 1L;

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
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getElements(java.lang.Object)
	 */
	@Override
	public Object[] getElements(Object inputElement)
	{
		return (Object[])inputElement; // new Object[] { ((Object[])inputElement)[0] }
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getChildren(java.lang.Object)
	 */
	@Override
	public Object[] getChildren(Object parentElement)
	{
		return ((PhysicalComponent)parentElement).getSubcomponents().toArray();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#getParent(java.lang.Object)
	 */
	@Override
	public Object getParent(Object element)
	{
		return ((PhysicalComponent)element).getParent();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITreeContentProvider#hasChildren(java.lang.Object)
	 */
	@Override
	public boolean hasChildren(Object element)
	{
		return ((PhysicalComponent)element).getSubcomponents().size() > 0;
	}

}
