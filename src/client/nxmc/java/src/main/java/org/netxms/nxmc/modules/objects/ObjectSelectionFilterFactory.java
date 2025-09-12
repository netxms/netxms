/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects;

import java.util.HashSet;
import java.util.Set;
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
         instance = new ObjectSelectionFilterFactory();
      return instance;
   }

   /**
    * Create filter for node selection - it allows node objects and possible parents - subnets and containers.
    * 
    * @return Class filter for node selection
    */
   public Set<Integer> createNodeSelectionFilter(boolean allowMobileDevices)
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(11);
      classFilter.add(AbstractObject.OBJECT_NETWORK);
      classFilter.add(AbstractObject.OBJECT_ZONE);
      classFilter.add(AbstractObject.OBJECT_SUBNET);
      classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
      classFilter.add(AbstractObject.OBJECT_COLLECTOR);
      classFilter.add(AbstractObject.OBJECT_CONTAINER);
      classFilter.add(AbstractObject.OBJECT_CLUSTER);
      classFilter.add(AbstractObject.OBJECT_RACK);
      classFilter.add(AbstractObject.OBJECT_CHASSIS);
      classFilter.add(AbstractObject.OBJECT_WIRELESSDOMAIN);
      classFilter.add(AbstractObject.OBJECT_NODE);
      if (allowMobileDevices)
      {
         classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
         classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
         classFilter.add(AbstractObject.OBJECT_SENSOR);
      }
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
      HashSet<Integer> classFilter = new HashSet<Integer>(2);
      classFilter.add(AbstractObject.OBJECT_NETWORKMAPROOT);
      classFilter.add(AbstractObject.OBJECT_NETWORKMAPGROUP);
      return classFilter;
   }

   /**
    * Create filter for asset group selection.
    * 
    * @return Class filter for asset group selection
    */
   public Set<Integer> createAssetGroupsSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(2);
      classFilter.add(AbstractObject.OBJECT_ASSETROOT);
      classFilter.add(AbstractObject.OBJECT_ASSETGROUP);
      return classFilter;
   }

   /**
    * Create filter for dashboard group selection.
    * 
    * @return Class filter for dashboard group selection
    */
   public Set<Integer> createDashboardGroupSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(2);
      classFilter.add(AbstractObject.OBJECT_DASHBOARDGROUP);
      classFilter.add(AbstractObject.OBJECT_DASHBOARDROOT);
      return classFilter;
   }

   /**
    * Create filter for dashboard selection.
    * 
    * @return Class filter for dashboard selection
    */
   public Set<Integer> createDashboardSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(4);
      classFilter.add(AbstractObject.OBJECT_DASHBOARD);
      classFilter.add(AbstractObject.OBJECT_DASHBOARDROOT);
      classFilter.add(AbstractObject.OBJECT_DASHBOARDGROUP);
      classFilter.add(AbstractObject.OBJECT_DASHBOARDTEMPLATE);
      return classFilter;
   }

   /**
    * Create filter for template selection - it allows template objects and possible parents - template groups.
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
    * Create filter for template group selection - it allows template groups.
    * 
    * @return Class filter for node selection
    */
   public Set<Integer> createTemplateGroupSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(2);
      classFilter.add(AbstractObject.OBJECT_TEMPLATEROOT);
      classFilter.add(AbstractObject.OBJECT_TEMPLATEGROUP);
      return classFilter;
   }

   /**
    * Create filter for data collection owner selection.
    * 
    * @return Class filter for data collection owner selection
    */
   public Set<Integer> createDataCollectionOwnerSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(14);
      classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
      classFilter.add(AbstractObject.OBJECT_NETWORK);
      classFilter.add(AbstractObject.OBJECT_ZONE);
      classFilter.add(AbstractObject.OBJECT_SUBNET);
      classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
      classFilter.add(AbstractObject.OBJECT_CIRCUIT);
      classFilter.add(AbstractObject.OBJECT_COLLECTOR);
      classFilter.add(AbstractObject.OBJECT_CONTAINER);
      classFilter.add(AbstractObject.OBJECT_CLUSTER);
      classFilter.add(AbstractObject.OBJECT_CHASSIS);
      classFilter.add(AbstractObject.OBJECT_RACK);
      classFilter.add(AbstractObject.OBJECT_NODE);
      classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
      classFilter.add(AbstractObject.OBJECT_SENSOR);
      classFilter.add(AbstractObject.OBJECT_TEMPLATEROOT);
      classFilter.add(AbstractObject.OBJECT_TEMPLATEGROUP);
      classFilter.add(AbstractObject.OBJECT_TEMPLATE);
      classFilter.add(AbstractObject.OBJECT_WIRELESSDOMAIN);
      return classFilter;
   }

   /**
    * Create filter for data collection targets selection.
    * 
    * @return Class filter for data collection targets selection
    */
   public Set<Integer> createDataCollectionTargetSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(16);
      classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
      classFilter.add(AbstractObject.OBJECT_NETWORK);
      classFilter.add(AbstractObject.OBJECT_ZONE);
      classFilter.add(AbstractObject.OBJECT_SUBNET);
      classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
      classFilter.add(AbstractObject.OBJECT_CIRCUIT);
      classFilter.add(AbstractObject.OBJECT_COLLECTOR);
      classFilter.add(AbstractObject.OBJECT_CONTAINER);
      classFilter.add(AbstractObject.OBJECT_RACK);
      classFilter.add(AbstractObject.OBJECT_CHASSIS);
      classFilter.add(AbstractObject.OBJECT_CLUSTER);
      classFilter.add(AbstractObject.OBJECT_NODE);
      classFilter.add(AbstractObject.OBJECT_MOBILEDEVICE);
      classFilter.add(AbstractObject.OBJECT_SENSOR);
      classFilter.add(AbstractObject.OBJECT_ACCESSPOINT);
      classFilter.add(AbstractObject.OBJECT_WIRELESSDOMAIN);
      return classFilter;
   }

   /**
    * Create filter for container selection
    * 
    * @return Class filter for container selection
    */
   public Set<Integer> createContainerSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(3);
      classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
      classFilter.add(AbstractObject.OBJECT_COLLECTOR);
      classFilter.add(AbstractObject.OBJECT_CONTAINER);
      return classFilter;
   }

   /**
    * Create filter for circuit selection - allows circuits and possible parents
    * 
    * @return Class filter for circuit selection
    */
   public Set<Integer> createCircuitSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(4);
      classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
      classFilter.add(AbstractObject.OBJECT_CIRCUIT);
      classFilter.add(AbstractObject.OBJECT_COLLECTOR);
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
      HashSet<Integer> classFilter = new HashSet<Integer>(3);
      classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICEROOT);
      classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICEPROTOTYPE);
      classFilter.add(AbstractObject.OBJECT_BUSINESSSERVICE);
      return classFilter;
   }

   /**
    * Create filter for dashboard and network map selection
    * 
    * @return
    */
   public Set<Integer> createDashboardAndNetworkMapSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(5);
      classFilter.add(AbstractObject.OBJECT_DASHBOARD);
      classFilter.add(AbstractObject.OBJECT_DASHBOARDGROUP);
      classFilter.add(AbstractObject.OBJECT_DASHBOARDROOT);
      classFilter.add(AbstractObject.OBJECT_DASHBOARDTEMPLATE);
      classFilter.add(AbstractObject.OBJECT_NETWORKMAP);
      classFilter.add(AbstractObject.OBJECT_NETWORKMAPGROUP);
      classFilter.add(AbstractObject.OBJECT_NETWORKMAPROOT);
      return classFilter;
   }

   /**
    * Create filter for rack selection
    * 
    * @return Class filter for rack selection
    */
   public Set<Integer> createRackSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(4);
      classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
      classFilter.add(AbstractObject.OBJECT_COLLECTOR);
      classFilter.add(AbstractObject.OBJECT_CONTAINER);
      classFilter.add(AbstractObject.OBJECT_RACK);
      return classFilter;
   }

   /**
    * Create filter for rack or chassis selection
    * 
    * @return Class filter for rack or chassis selection
    */
   public Set<Integer> createRackOrChassisSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(4);
      classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
      classFilter.add(AbstractObject.OBJECT_COLLECTOR);
      classFilter.add(AbstractObject.OBJECT_CONTAINER);
      classFilter.add(AbstractObject.OBJECT_RACK);
      classFilter.add(AbstractObject.OBJECT_CHASSIS);
      return classFilter;
   }

   /**
    * Create filter for asset selection
    * 
    * @return Class filter for asset selection
    */
   public Set<Integer> createAssetSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(4);
      classFilter.add(AbstractObject.OBJECT_ASSETROOT);
      classFilter.add(AbstractObject.OBJECT_ASSETGROUP);
      classFilter.add(AbstractObject.OBJECT_ASSET);
      return classFilter;
   }

   public Set<Integer> createInterfaceSelectionFilter()
   {
      HashSet<Integer> classFilter = new HashSet<Integer>(12);
      classFilter.add(AbstractObject.OBJECT_NETWORK);
      classFilter.add(AbstractObject.OBJECT_ZONE);
      classFilter.add(AbstractObject.OBJECT_SUBNET);
      classFilter.add(AbstractObject.OBJECT_SERVICEROOT);
      classFilter.add(AbstractObject.OBJECT_CIRCUIT);
      classFilter.add(AbstractObject.OBJECT_COLLECTOR);
      classFilter.add(AbstractObject.OBJECT_CONTAINER);
      classFilter.add(AbstractObject.OBJECT_RACK);
      classFilter.add(AbstractObject.OBJECT_CHASSIS);
      classFilter.add(AbstractObject.OBJECT_CLUSTER);
      classFilter.add(AbstractObject.OBJECT_NODE);
      classFilter.add(AbstractObject.OBJECT_WIRELESSDOMAIN);
      classFilter.add(AbstractObject.OBJECT_INTERFACE);
      return classFilter;
   }
}
