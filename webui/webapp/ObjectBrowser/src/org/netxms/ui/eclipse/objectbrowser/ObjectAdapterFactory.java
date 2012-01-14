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
package org.netxms.ui.eclipse.objectbrowser;

import java.util.Iterator;

import org.eclipse.core.runtime.IAdapterFactory;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.ui.model.IWorkbenchAdapter;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.ServiceCheck;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Adapter factory for NetXMS objects
 *
 */
public class ObjectAdapterFactory implements IAdapterFactory
{
	@SuppressWarnings("rawtypes")
	private static final Class[] supportedClasses = 
	{
		IWorkbenchAdapter.class
	};

	private boolean showComments;

	/**
	 * Default constructor
	 */
	public ObjectAdapterFactory()
	{
		showComments = Activator.getDefault().getPreferenceStore().getBoolean("SHOW_COMMENTS"); //$NON-NLS-1$
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

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.IAdapterFactory#getAdapter(java.lang.Object, java.lang.Class)
	 */
	@SuppressWarnings("rawtypes")
	@Override
	public Object getAdapter(Object adaptableObject, Class adapterType)
	{
		if ((adapterType == IWorkbenchAdapter.class) &&  (adaptableObject instanceof GenericObject))
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
							return Activator.getImageDescriptor("icons/network.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_SERVICEROOT:
							return Activator.getImageDescriptor("icons/service_root.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_CONTAINER:
							return Activator.getImageDescriptor("icons/container.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_ZONE:
							return Activator.getImageDescriptor("icons/zone.gif"); //$NON-NLS-1$
						case GenericObject.OBJECT_SUBNET:
							return Activator.getImageDescriptor("icons/subnet.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_CLUSTER:
							return Activator.getImageDescriptor("icons/cluster.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_NODE:
							return Activator.getImageDescriptor("icons/node.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_INTERFACE:
							return Activator.getImageDescriptor("icons/interface.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_NETWORKSERVICE:
							return Activator.getImageDescriptor("icons/network_service.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_CONDITION:
							return Activator.getImageDescriptor("icons/condition.gif"); //$NON-NLS-1$
						case GenericObject.OBJECT_TEMPLATEROOT:
							return Activator.getImageDescriptor("icons/template_root.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_TEMPLATEGROUP:
							return Activator.getImageDescriptor("icons/template_group.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_TEMPLATE:
							return Activator.getImageDescriptor("icons/template.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_POLICYROOT:
							return Activator.getImageDescriptor("icons/policy_root.gif"); //$NON-NLS-1$
						case GenericObject.OBJECT_POLICYGROUP:
							return Activator.getImageDescriptor("icons/policy_group.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_AGENTPOLICY:
						case GenericObject.OBJECT_AGENTPOLICY_CONFIG:
							return Activator.getImageDescriptor("icons/policy.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_NETWORKMAP:
							return Activator.getImageDescriptor("icons/netmap.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_NETWORKMAPGROUP:
							return Activator.getImageDescriptor("icons/netmap_group.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_NETWORKMAPROOT:
							return Activator.getImageDescriptor("icons/netmap_root.gif"); //$NON-NLS-1$
						case GenericObject.OBJECT_DASHBOARD:
							return Activator.getImageDescriptor("icons/dashboard.gif"); //$NON-NLS-1$
						case GenericObject.OBJECT_DASHBOARDROOT:
							return Activator.getImageDescriptor("icons/dashboard_root.gif"); //$NON-NLS-1$
						case GenericObject.OBJECT_REPORT:
							return Activator.getImageDescriptor("icons/report.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_REPORTGROUP:
							return Activator.getImageDescriptor("icons/report_group.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_REPORTROOT:
							return Activator.getImageDescriptor("icons/report_root.gif"); //$NON-NLS-1$
						case GenericObject.OBJECT_BUSINESSSERVICEROOT:
						case GenericObject.OBJECT_BUSINESSSERVICE:
							return Activator.getImageDescriptor("icons/business_service.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_NODELINK:
							return Activator.getImageDescriptor("icons/node_link.png"); //$NON-NLS-1$
						case GenericObject.OBJECT_SLMCHECK:
							return Activator.getImageDescriptor(((ServiceCheck)object).isTemplate() ? "icons/service_check_template.gif" : "icons/service_check.gif"); //$NON-NLS-1$
						default:
							return null;
					}
				}

				@Override
				public String getLabel(Object o)
				{
					if (showComments)
					{
						final String comments = ((GenericObject)o).getComments();
						if (comments != null)
						{
							final String s = comments.replace("\r", "").replace("\n", " ").trim(); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$
							if (s.length() > 0)
							{
								return ((GenericObject)o).getObjectName() + " [" + ((s.length() > 100) ? s.substring(0, 100) : s) + "]"; //$NON-NLS-1$ //$NON-NLS-2$
							}
						}
					}
					return ((GenericObject)o).getObjectName();
				}

				@Override
				public Object getParent(Object o)
				{
					NXCSession session = (NXCSession)ConsoleSharedData.getSession();
					if (session != null)
					{
						Iterator<Long> it = ((GenericObject)o).getParents();
						return it.hasNext() ? session.findObjectById(it.next()) : null;
					}
					return null;
				}
			};
		}
		return null;
	}
}
