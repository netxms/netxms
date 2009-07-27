/**
 * 
 */
package org.netxms.ui.eclipse.usermanager;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.client.NXCAccessListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.NXCUser;
import org.netxms.client.NXCUserDBObject;
import org.netxms.client.NXCUserGroup;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * Adapter factory for NXCUserDBObject and derived classes
 * 
 * @author Victor
 *
 */
public class UserAdapterFactory implements IAdapterFactory
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
			if (adaptableObject instanceof NXCUser)
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
						return Activator.getImageDescriptor("icons/user.png");
					}

					@Override
					public String getLabel(Object o)
					{
						return ((NXCUser)o).getName();
					}

					@Override
					public Object getParent(Object o)
					{
						return null;
					}
				};
			}

			// NXCUserGroup
			if (adaptableObject instanceof NXCUserGroup)
			{
				return new IWorkbenchAdapter() {
					@Override
					public Object[] getChildren(Object o)
					{
						long[] members = ((NXCUserGroup)o).getMembers();
						NXCUserDBObject[] childrens = new NXCUser[members.length];
						for(int i = 0; i < members.length; i++)
							childrens[i] = NXMCSharedData.getInstance().getSession().findUserDBObjectById(members[i]);
						return childrens;
					}

					@Override
					public ImageDescriptor getImageDescriptor(Object object)
					{
						return Activator.getImageDescriptor("icons/group.png");
					}

					@Override
					public String getLabel(Object o)
					{
						return ((NXCUserGroup)o).getName();
					}

					@Override
					public Object getParent(Object o)
					{
						return null;
					}
				};
			}

			// NXCAccessListElement
			if (adaptableObject instanceof NXCAccessListElement)
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
						long userId = ((NXCAccessListElement)object).getUserId();
						return Activator.getImageDescriptor((userId < 0x80000000L) ? "icons/user.png" : "icons/group.png");
					}

					@Override
					public String getLabel(Object object)
					{
						long userId = ((NXCAccessListElement)object).getUserId();
						NXCSession session = NXMCSharedData.getInstance().getSession();
						NXCUserDBObject dbo = session.findUserDBObjectById(userId);
						return (dbo != null) ? dbo.getName() : ("{" + Long.toString(userId) + "}");
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
