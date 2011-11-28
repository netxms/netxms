/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.actionmanager;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.client.ServerAction;

/**
 * Adapter factory for server actions
 *
 */
public class ServerActionAdapterFactory implements IAdapterFactory
{
	@SuppressWarnings("rawtypes")
	private static final Class[] supportedClasses = 
	{
		IWorkbenchAdapter.class
	};

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Object getAdapter(Object adaptableObject, Class adapterType)
	{
		if ((adapterType != IWorkbenchAdapter.class) ||
		    !(adaptableObject instanceof ServerAction))
			return null;
		
		return new IWorkbenchAdapter() {
			@Override
			public Object[] getChildren(Object o)
			{
				return null;
			}

			@Override
			public ImageDescriptor getImageDescriptor(Object object)
			{
				switch(((ServerAction)object).getType())
				{
					case ServerAction.EXEC_LOCAL:
						return Activator.getImageDescriptor("icons/exec_local.png"); //$NON-NLS-1$
					case ServerAction.EXEC_REMOTE:
						return Activator.getImageDescriptor("icons/exec_remote.png"); //$NON-NLS-1$
					case ServerAction.SEND_EMAIL:
						return Activator.getImageDescriptor("icons/email.png"); //$NON-NLS-1$
					case ServerAction.SEND_SMS:
						return Activator.getImageDescriptor("icons/sms.png"); //$NON-NLS-1$
					case ServerAction.FORWARD_EVENT:
						return Activator.getImageDescriptor("icons/fwd_event.png"); //$NON-NLS-1$
				}
				return null;
			}

			@Override
			public String getLabel(Object o)
			{
				return ((ServerAction)o).getName();
			}

			@Override
			public Object getParent(Object o)
			{
				return null;
			}
		};
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
