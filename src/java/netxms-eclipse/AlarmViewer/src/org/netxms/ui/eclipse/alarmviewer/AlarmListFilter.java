/**
 * 
 */
package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author victor
 *
 */
public class AlarmListFilter extends ViewerFilter
{
	private long rootObject;
	
	/**
	 * 
	 */
	public AlarmListFilter()
	{
		rootObject = 0;
	}

	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
		if ((rootObject == 0) || (rootObject == ((Alarm)element).getSourceObjectId()))
			return true;	// No filtering by object ID or root object is a source
		
		GenericObject object = NXMCSharedData.getInstance().getSession().findObjectById(((Alarm)element).getSourceObjectId());
		if (object != null)
			return object.isChildOf(rootObject);
		return false;
	}

	
	/**
	 * @param rootObject the rootObject to set
	 */
	public final void setRootObject(long rootObject)
	{
		this.rootObject = rootObject;
	}
}
