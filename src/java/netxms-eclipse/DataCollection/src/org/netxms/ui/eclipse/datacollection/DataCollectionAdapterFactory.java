/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.client.datacollection.DataCollectionItem;

/**
 * Adapter factory for data collection objects
 * 
 */
public class DataCollectionAdapterFactory implements IAdapterFactory
{
	@SuppressWarnings("rawtypes")
	private static final Class[] supportedClasses = 
	{
		IWorkbenchAdapter.class
	};
	
	private static final String[] dciStatusImages = { "icons/active.gif", "icons/disabled.gif", "icons/unsupported.gif" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
	 */
	@SuppressWarnings("rawtypes")
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
	@SuppressWarnings("rawtypes")
	@Override
	public Class[] getAdapterList()
	{
		return supportedClasses;
	}
}
