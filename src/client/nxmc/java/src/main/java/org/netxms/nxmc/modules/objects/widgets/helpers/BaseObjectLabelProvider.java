/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.ServiceCheck;
import org.netxms.client.objects.UnknownObject;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Base label provider for NetXMS objects
 */
public class BaseObjectLabelProvider extends LabelProvider
{
   private static Map<Integer, Image> images;
   private static Image serviceCheckTemplate;

   static
   {
      images = new HashMap<Integer, Image>();
      images.put(AbstractObject.OBJECT_ACCESSPOINT, ResourceManager.getImageDescriptor("icons/objects/access_point.png").createImage());
      images.put(AbstractObject.OBJECT_BUSINESSSERVICE, ResourceManager.getImageDescriptor("icons/objects/business_service.png").createImage());
      images.put(AbstractObject.OBJECT_BUSINESSSERVICEROOT, ResourceManager.getImageDescriptor("icons/objects/business_service.png").createImage());
      images.put(AbstractObject.OBJECT_CHASSIS, ResourceManager.getImageDescriptor("icons/objects/chassis.png").createImage());
      images.put(AbstractObject.OBJECT_CLUSTER, ResourceManager.getImageDescriptor("icons/objects/cluster.png").createImage());
      images.put(AbstractObject.OBJECT_CONDITION, ResourceManager.getImageDescriptor("icons/objects/condition.gif").createImage());
      images.put(AbstractObject.OBJECT_CONTAINER, ResourceManager.getImageDescriptor("icons/objects/container.png").createImage());
      images.put(AbstractObject.OBJECT_DASHBOARD, ResourceManager.getImageDescriptor("icons/objects/dashboard.gif").createImage());
      images.put(AbstractObject.OBJECT_DASHBOARDGROUP, ResourceManager.getImageDescriptor("icons/objects/dashboard_group.png").createImage());
      images.put(AbstractObject.OBJECT_DASHBOARDROOT, ResourceManager.getImageDescriptor("icons/objects/dashboard_root.gif").createImage());
      images.put(AbstractObject.OBJECT_INTERFACE, ResourceManager.getImageDescriptor("icons/objects/interface.png").createImage());
      images.put(AbstractObject.OBJECT_MOBILEDEVICE, ResourceManager.getImageDescriptor("icons/objects/mobile_device.png").createImage());
      images.put(AbstractObject.OBJECT_NETWORK, ResourceManager.getImageDescriptor("icons/objects/network.png").createImage());
      images.put(AbstractObject.OBJECT_NETWORKMAP, ResourceManager.getImageDescriptor("icons/objects/netmap.png").createImage());
      images.put(AbstractObject.OBJECT_NETWORKMAPGROUP, ResourceManager.getImageDescriptor("icons/objects/netmap_group.png").createImage());
      images.put(AbstractObject.OBJECT_NETWORKMAPROOT, ResourceManager.getImageDescriptor("icons/objects/netmap_root.gif").createImage());
      images.put(AbstractObject.OBJECT_NETWORKSERVICE, ResourceManager.getImageDescriptor("icons/objects/network_service.png").createImage());
      images.put(AbstractObject.OBJECT_NODE, ResourceManager.getImageDescriptor("icons/objects/node.png").createImage());
      images.put(AbstractObject.OBJECT_RACK, ResourceManager.getImageDescriptor("icons/objects/rack.gif").createImage());
      images.put(AbstractObject.OBJECT_SENSOR, ResourceManager.getImageDescriptor("icons/objects/sensor.gif").createImage());
      images.put(AbstractObject.OBJECT_SERVICEROOT, ResourceManager.getImageDescriptor("icons/objects/service_root.png").createImage());
      images.put(AbstractObject.OBJECT_SLMCHECK, ResourceManager.getImageDescriptor("icons/objects/service_check.gif").createImage());
      images.put(AbstractObject.OBJECT_SUBNET, ResourceManager.getImageDescriptor("icons/objects/subnet.png").createImage());
      images.put(AbstractObject.OBJECT_TEMPLATE, ResourceManager.getImageDescriptor("icons/objects/template.png").createImage());
      images.put(AbstractObject.OBJECT_TEMPLATEGROUP, ResourceManager.getImageDescriptor("icons/objects/template_group.png").createImage());
      images.put(AbstractObject.OBJECT_TEMPLATEROOT, ResourceManager.getImageDescriptor("icons/objects/template_root.png").createImage());
      images.put(AbstractObject.OBJECT_VPNCONNECTOR, ResourceManager.getImageDescriptor("icons/objects/vpn.png").createImage());
      images.put(AbstractObject.OBJECT_ZONE, ResourceManager.getImageDescriptor("icons/objects/zone.gif").createImage());
      serviceCheckTemplate = ResourceManager.getImageDescriptor("icons/objects/service_check_template.gif").createImage();
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
    */
   @Override
   public Image getImage(Object element)
   {
      if (element instanceof UnknownObject)
         return SharedIcons.IMG_UNKNOWN_OBJECT;
      
      if ((element instanceof ServiceCheck) && ((ServiceCheck)element).isTemplate())
         return serviceCheckTemplate;
      
      Image image = images.get(((AbstractObject)element).getObjectClass());
      return (image != null) ? image : SharedIcons.IMG_UNKNOWN_OBJECT;
   }

   /**
    * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
    */
   @Override
   public String getText(Object element)
   {
      return ((AbstractObject)element).getObjectName();
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      super.dispose();
   }
}
