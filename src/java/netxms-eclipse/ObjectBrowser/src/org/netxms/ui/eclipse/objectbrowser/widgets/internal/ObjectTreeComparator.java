/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser.widgets.internal;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.netxms.client.objects.GenericObject;

/**
 * @author Victor
 *
 */
public class ObjectTreeComparator extends ViewerComparator
{
	// Categories
	private static final int CATEGORY_CONTAINER = 10;
	private static final int CATEGORY_OTHER = 255;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#category(java.lang.Object)
	 */
	@Override
	public int category(Object element)
	{
		if (((GenericObject)element).getObjectId() < 10)
			return (int)((GenericObject)element).getObjectId();
		if ((((GenericObject)element).getObjectClass() == GenericObject.OBJECT_CONTAINER) ||
		    (((GenericObject)element).getObjectClass() == GenericObject.OBJECT_TEMPLATEGROUP) ||
		    (((GenericObject)element).getObjectClass() == GenericObject.OBJECT_POLICYGROUP))
			return CATEGORY_CONTAINER;
		return CATEGORY_OTHER;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		return super.compare(viewer, e1, e2);
	}
}
