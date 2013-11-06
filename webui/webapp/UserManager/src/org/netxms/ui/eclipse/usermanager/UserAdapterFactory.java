/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.usermanager;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.api.client.users.User;
import org.netxms.api.client.users.UserGroup;
import org.netxms.api.client.users.UserManager;
import org.netxms.api.client.users.AbstractAccessListElement;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Adapter factory for NXCUserDBObject and derived classes
 * 
 */
public class UserAdapterFactory implements IAdapterFactory
{
	@SuppressWarnings("rawtypes")
	private static final Class[] supportedClasses = 
	{
		IWorkbenchAdapter.class
	};

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapterList()
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Class[] getAdapterList()
	{
		return supportedClasses;
	}
	
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
			if (adaptableObject instanceof User)
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
						return Activator.getImageDescriptor("icons/user.png"); //$NON-NLS-1$
					}

					@Override
					public String getLabel(Object o)
					{
						return ((User)o).getName();
					}

					@Override
					public Object getParent(Object o)
					{
						return null;
					}
				};
			}

			// NXCUserGroup
			if (adaptableObject instanceof UserGroup)
			{
				return new IWorkbenchAdapter() {
					@Override
					public Object[] getChildren(Object o)
					{
						long[] members = ((UserGroup)o).getMembers();
						AbstractUserObject[] childrens = new User[members.length];
						for(int i = 0; i < members.length; i++)
							childrens[i] = ((UserManager)ConsoleSharedData.getSession()).findUserDBObjectById(members[i]);
						return childrens;
					}

					@Override
					public ImageDescriptor getImageDescriptor(Object object)
					{
						return Activator.getImageDescriptor("icons/group.png"); //$NON-NLS-1$
					}

					@Override
					public String getLabel(Object o)
					{
						return ((UserGroup)o).getName();
					}

					@Override
					public Object getParent(Object o)
					{
						return null;
					}
				};
			}

			// AccessListElement
			if (adaptableObject instanceof AbstractAccessListElement)
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
						long userId = ((AbstractAccessListElement)object).getUserId();
						return Activator.getImageDescriptor((userId < 0x80000000L) ? "icons/user.png" : "icons/group.png"); //$NON-NLS-1$ //$NON-NLS-2$
					}

					@Override
					public String getLabel(Object object)
					{
						long userId = ((AbstractAccessListElement)object).getUserId();
						UserManager umgr = (UserManager)ConsoleSharedData.getSession();
						AbstractUserObject dbo = umgr.findUserDBObjectById(userId);
						return (dbo != null) ? dbo.getName() : ("{" + Long.toString(userId) + "}"); //$NON-NLS-1$ //$NON-NLS-2$
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
