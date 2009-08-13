/**
 * 
 */
package org.netxms.ui.eclipse.datacollection;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.client.datacollection.DataCollectionItem;

/**
 * Adapter factory for data collection objects
 * 
 * @author Victor Kirhenshtein
 *
 */
public class DataCollectionAdapterFactory implements IAdapterFactory
{
	@SuppressWarnings("unchecked")
	private static final Class[] supportedClasses = 
	{
		IWorkbenchAdapter.class
	};
	
	private static final String[] dciStatusImages = { "icons/active.gif", "icons/disabled.gif", "icons/unsupported.gif" };

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
			if (adaptableObject instanceof DataCollectionItem)
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
						return Activator.getImageDescriptor(dciStatusImages[((DataCollectionItem)object).getStatus()]);
					}

					@Override
					public String getLabel(Object o)
					{
						return ((DataCollectionItem)o).getDescription();
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

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapterList()
	 */
	@SuppressWarnings("unchecked")
	@Override
	public Class[] getAdapterList()
	{
		return supportedClasses;
	}
}
