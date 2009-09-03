/**
 * 
 */
package org.netxms.ui.eclipse.eventmanager;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.shared.StatusDisplayInfo;

/**
 * Adapter factory for NXCUserDBObject and derived classes
 * 
 * @author Victor
 *
 */
public class EventTemplateAdapterFactory implements IAdapterFactory
{
	@SuppressWarnings("unchecked")
	private static final Class[] supportedClasses = 
	{
		IWorkbenchAdapter.class
	};

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapterList()
	 */
	@SuppressWarnings("unchecked")
	@Override
	public Class[] getAdapterList()
	{
		return supportedClasses;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public Object getAdapter(Object adaptableObject, Class adapterType)
	{
		if (adapterType == IWorkbenchAdapter.class)
		{
			// NXCUser
			if (adaptableObject instanceof EventTemplate)
			{
				return new IWorkbenchAdapter() {
					@Override
					public Object[] getChildren(Object o)
					{
						return null;
					}

					@Override
					public ImageDescriptor getImageDescriptor(Object object)
					{
						return StatusDisplayInfo.getStatusImageDescriptor(((EventTemplate)object).getSeverity());
					}

					@Override
					public String getLabel(Object o)
					{
						return ((EventTemplate)o).getName();
					}

					@Override
					public Object getParent(Object o)
					{
						return null;
					}
				};
			}
		}
		return null;
	}
}
