/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser;

import java.util.Iterator;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author Victor
 *
 */
public class ObjectAdapterFactory implements IAdapterFactory
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
			if (adaptableObject instanceof GenericObject)
			{
				return new IWorkbenchAdapter() {
					@Override
					public Object[] getChildren(Object o)
					{
						return ((GenericObject)o).getChildsAsArray();
					}

					@Override
					public ImageDescriptor getImageDescriptor(Object object)
					{
						switch(((GenericObject)object).getObjectClass())
						{
							case GenericObject.OBJECT_NETWORK:
								return Activator.getImageDescriptor("icons/network.png");
							case GenericObject.OBJECT_SERVICEROOT:
								return Activator.getImageDescriptor("icons/service_root.png");
							case GenericObject.OBJECT_CONTAINER:
								return Activator.getImageDescriptor("icons/container.png");
							case GenericObject.OBJECT_SUBNET:
								return Activator.getImageDescriptor("icons/subnet.png");
							case GenericObject.OBJECT_NODE:
								return Activator.getImageDescriptor("icons/node.png");
							case GenericObject.OBJECT_INTERFACE:
								return Activator.getImageDescriptor("icons/interface.png");
							case GenericObject.OBJECT_CONDITION:
								return Activator.getImageDescriptor("icons/condition.png");
							case GenericObject.OBJECT_TEMPLATEROOT:
								return Activator.getImageDescriptor("icons/template_root.png");
							case GenericObject.OBJECT_TEMPLATEGROUP:
								return Activator.getImageDescriptor("icons/template_group.png");
							case GenericObject.OBJECT_TEMPLATE:
								return Activator.getImageDescriptor("icons/template.png");
							case GenericObject.OBJECT_POLICYROOT:
								return Activator.getImageDescriptor("icons/policy_root.png");
							case GenericObject.OBJECT_POLICYGROUP:
								return Activator.getImageDescriptor("icons/policy_group.png");
							case GenericObject.OBJECT_AGENTPOLICY:
							case GenericObject.OBJECT_AGENTPOLICY_CONFIG:
								return Activator.getImageDescriptor("icons/policy.png");
							default:
								return null;
						}
					}

					@Override
					public String getLabel(Object o)
					{
						return ((GenericObject)o).getObjectName();
					}

					@Override
					public Object getParent(Object o)
					{
						NXCSession session = NXMCSharedData.getInstance().getSession();
						if (session != null)
						{
							Iterator<Long> it = ((GenericObject)o).getParents();
							return it.hasNext() ? session.findObjectById(it.next()) : null;
						}
						return null;
					}
				};
			}
		}
		return null;
	}
}
