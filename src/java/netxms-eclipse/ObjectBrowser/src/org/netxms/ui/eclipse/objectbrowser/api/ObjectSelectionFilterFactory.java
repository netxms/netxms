/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.api;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.netxms.client.objects.AbstractObject;

/**
 * Helper class to create object selection filters
 */
public class ObjectSelectionFilterFactory
{
	private static ObjectSelectionFilterFactory instance;
	
	/**
	 * @return the instance
	 */
	public static ObjectSelectionFilterFactory getInstance()
	{
		if (instance == null)
		{
			// Read extension if defined
			final IExtensionRegistry reg = Platform.getExtensionRegistry();
			IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.objectbrowser.objectSelectionFilterFactories"); //$NON-NLS-1$
			for(int i = 0; i < elements.length; i++)
			{
				try
				{
					instance = (ObjectSelectionFilterFactory)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
					break;
				}
				catch(CoreException e)
				{
					e.printStackTrace();
				}
			}
			if (instance == null)
				instance = new ObjectSelectionFilterFactory();	// use default implementation
		}
		return instance;
	}

	/**
	 * Create filter for node selection - it allows node objects and possible
	 * parents - subnets and containers.
	 * 
	 * @return Class filter for node selection
	 */
	public Set<Integer> createNodeSelectionFilter(boolean allowMobileDevices)
	{
		HashSet<Integer> classFilter = new HashSet<Integer>(7);
		classFilter.add(AbstractObject.OBJECT_NETWORK);
		classFilter.add(AbstractObject.OBJECT_ZONE);
		classFilter.add(AbstractObject.OBJECT_SUBNET);
		classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
		classFilter.add(AbstractObject.OBJECT_CONTAINER);
		classFilter.add(AbstractObject.OBJECT_CLUSTER);
		classFilter.add(AbstractObject.OBJECT_RACK);
		classFilter.add(AbstractObject.OBJECT_NODE);
		if (allowMobileDevices)
			classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
		return classFilter;
	}

	/**
	 * Create filter for zone selection - it allows only zone and entire network obejcts.
	 * 
	 * @return Class filter for zone selection
	 */
	public Set<Integer> createZoneSelectionFilter()
	{
		HashSet<Integer> classFilter = new HashSet<Integer>(2);
		classFilter.add(AbstractObject.OBJECT_NETWORK);
		classFilter.add(AbstractObject.OBJECT_ZONE);
		return classFilter;
	}

	/**
	 * Create filter for network map selection.
	 * 
	 * @return Class filter for network map selection
	 */
	public Set<Integer> createNetworkMapSelectionFilter()
	{
		HashSet<Integer> classFilter = new HashSet<Integer>(3);
		classFilter.add(AbstractObject.OBJECT_NETWORKMAPROOT);
		classFilter.add(AbstractObject.OBJECT_NETWORKMAPGROUP);
		classFilter.add(AbstractObject.OBJECT_NETWORKMAP);
		return classFilter;
	}
	
	/**
    * Create filter for network map group selection.
    * 
    * @return Class filter for network map group selection
    */
   public Set<Integer> createNetworkMapGroupsSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(3);
      classFilter.add(AbstractObject.OBJECT_NETWORKMAPROOT);
      classFilter.add(AbstractObject.OBJECT_NETWORKMAPGROUP);
      return classFilter;
   }
	
   /**
    * Create filter for policies group selection.
    * 
    * @return Class filter for policies group selection
    */
   public Set<Integer> createPolicySelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(3);
      classFilter.add(AbstractObject.OBJECT_POLICYROOT);
      classFilter.add(AbstractObject.OBJECT_POLICYGROUP);      
      return classFilter;
   }
	
   /**
    * Create filter for dashboard selection.
    * 
    * @return Class filter for dashboard selection
    */
   public Set<Integer> createDashboardSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(3);
      classFilter.add(AbstractObject.OBJECT_DASHBOARD);
      classFilter.add(AbstractObject.OBJECT_DASHBOARDROOT);
      return classFilter;
   }

	/**
	 * Create filter for template selection - it allows template objects and possible
	 * parents - template groups.
	 * 
	 * @return Class filter for node selection
	 */
	public Set<Integer> createTemplateSelectionFilter()
	{
		HashSet<Integer> classFilter = new HashSet<Integer>(3);
		classFilter.add(AbstractObject.OBJECT_TEMPLATEROOT);
		classFilter.add(AbstractObject.OBJECT_TEMPLATEGROUP);
		classFilter.add(AbstractObject.OBJECT_TEMPLATE);
		return classFilter;
	}

	/**
	 * Create filter for node selection - it allows node objects and possible
	 * parents - subnets and containers.
	 * 
	 * @return Class filter for node selection
	 */
	public Set<Integer> createNodeAndTemplateSelectionFilter(boolean allowMobileDevices)
	{
		HashSet<Integer> classFilter = new HashSet<Integer>(9);
		classFilter.add(AbstractObject.OBJECT_NETWORK);
		classFilter.add(AbstractObject.OBJECT_SUBNET);
		classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
		classFilter.add(AbstractObject.OBJECT_CONTAINER);
		classFilter.add(AbstractObject.OBJECT_CLUSTER);
		classFilter.add(AbstractObject.OBJECT_NODE);
		classFilter.add(AbstractObject.OBJECT_TEMPLATEROOT);
		classFilter.add(AbstractObject.OBJECT_TEMPLATEGROUP);
		classFilter.add(AbstractObject.OBJECT_TEMPLATE);
		if (allowMobileDevices)
			classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
		return classFilter;
	}

	/**
	 * Create filter for node selection - it allows node objects and possible
	 * parents - subnets and containers.
	 * 
	 * @return Class filter for node selection
	 */
	public Set<Integer> createContainerSelectionFilter()
	{
		HashSet<Integer> classFilter = new HashSet<Integer>(2);
		classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
		classFilter.add(AbstractObject.OBJECT_CONTAINER);
		return classFilter;
	}

	/**
	 * Create filter for business service selection.
	 * 
	 * @return Class filter for node selection
	 */
	public Set<Integer> createBusinessServiceSelectionFilter()
	{
		HashSet<Integer> classFilter = new HashSet<Integer>(2);
		classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICEROOT);
		classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICE);
		return classFilter;
	}
}
