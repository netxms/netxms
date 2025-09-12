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

import java.util.HashMap;
import java.util.Map;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;

/**
 * Storage for object icons
 */
public final class ObjectIcons
{
   /**
    * Initialize object icon storage
    * 
    * @param display curent display
    */
   public static void init(Display display)
   {
      Registry.setSingleton(ObjectIcons.class, new ObjectIcons(display));
   }

   private Map<Integer, Image> images;

   /**
    * Initialize object icons storage.
    *
    * @param display current display
    */
   private ObjectIcons(Display display)
   {
      images = new HashMap<Integer, Image>();
      images.put(AbstractObject.OBJECT_ACCESSPOINT, ResourceManager.getImage(display, "icons/objects/access_point.png"));
      images.put(AbstractObject.OBJECT_ASSET, ResourceManager.getImage(display, "icons/objects/asset.png"));
      images.put(AbstractObject.OBJECT_ASSETGROUP, ResourceManager.getImage(display, "icons/objects/asset-group.png"));
      images.put(AbstractObject.OBJECT_ASSETROOT, ResourceManager.getImage(display, "icons/objects/asset-root.png"));
      images.put(AbstractObject.OBJECT_BUSINESSSERVICE, ResourceManager.getImage(display, "icons/objects/business_service.png"));
      images.put(AbstractObject.OBJECT_BUSINESSSERVICEPROTOTYPE, ResourceManager.getImage(display, "icons/objects/business_service.png"));
      images.put(AbstractObject.OBJECT_BUSINESSSERVICEROOT, ResourceManager.getImage(display, "icons/objects/business_service.png"));
      images.put(AbstractObject.OBJECT_CHASSIS, ResourceManager.getImage(display, "icons/objects/chassis.png"));
      images.put(AbstractObject.OBJECT_CIRCUIT, ResourceManager.getImage(display, "icons/objects/circuit.png")); //TODO: create image
      images.put(AbstractObject.OBJECT_CLUSTER, ResourceManager.getImage(display, "icons/objects/cluster.png"));
      images.put(AbstractObject.OBJECT_COLLECTOR, ResourceManager.getImage(display, "icons/objects/collector.png"));
      images.put(AbstractObject.OBJECT_CONDITION, ResourceManager.getImage(display, "icons/objects/condition.gif"));
      images.put(AbstractObject.OBJECT_CONTAINER, ResourceManager.getImage(display, "icons/objects/container.png"));
      images.put(AbstractObject.OBJECT_DASHBOARD, ResourceManager.getImage(display, "icons/objects/dashboard.png"));
      images.put(AbstractObject.OBJECT_DASHBOARDGROUP, ResourceManager.getImage(display, "icons/objects/dashboard_group.png"));
      images.put(AbstractObject.OBJECT_DASHBOARDROOT, ResourceManager.getImage(display, "icons/objects/dashboard_root.gif"));
      images.put(AbstractObject.OBJECT_DASHBOARDTEMPLATE, ResourceManager.getImage(display, "icons/objects/dashboard.png")); //TODO: create image for dashboard template
      images.put(AbstractObject.OBJECT_INTERFACE, ResourceManager.getImage(display, "icons/objects/interface.png"));
      images.put(AbstractObject.OBJECT_MOBILEDEVICE, ResourceManager.getImage(display, "icons/objects/mobile_device.png"));
      images.put(AbstractObject.OBJECT_NETWORK, ResourceManager.getImage(display, "icons/objects/network.png"));
      images.put(AbstractObject.OBJECT_NETWORKMAP, ResourceManager.getImage(display, "icons/objects/netmap.png"));
      images.put(AbstractObject.OBJECT_NETWORKMAPGROUP, ResourceManager.getImage(display, "icons/objects/netmap_group.png"));
      images.put(AbstractObject.OBJECT_NETWORKMAPROOT, ResourceManager.getImage(display, "icons/objects/netmap_root.gif"));
      images.put(AbstractObject.OBJECT_NETWORKSERVICE, ResourceManager.getImage(display, "icons/objects/network_service.png"));
      images.put(AbstractObject.OBJECT_NODE, ResourceManager.getImage(display, "icons/objects/node.png"));
      images.put(AbstractObject.OBJECT_RACK, ResourceManager.getImage(display, "icons/objects/rack.gif"));
      images.put(AbstractObject.OBJECT_SENSOR, ResourceManager.getImage(display, "icons/objects/sensor.gif"));
      images.put(AbstractObject.OBJECT_SERVICEROOT, ResourceManager.getImage(display, "icons/objects/service_root.png"));
      images.put(AbstractObject.OBJECT_SUBNET, ResourceManager.getImage(display, "icons/objects/subnet.png"));
      images.put(AbstractObject.OBJECT_TEMPLATE, ResourceManager.getImage(display, "icons/objects/template.png"));
      images.put(AbstractObject.OBJECT_TEMPLATEGROUP, ResourceManager.getImage(display, "icons/objects/template_group.png"));
      images.put(AbstractObject.OBJECT_TEMPLATEROOT, ResourceManager.getImage(display, "icons/objects/template_root.png"));
      images.put(AbstractObject.OBJECT_VPNCONNECTOR, ResourceManager.getImage(display, "icons/objects/vpn.png"));
      images.put(AbstractObject.OBJECT_WIRELESSDOMAIN, ResourceManager.getImage(display, "icons/objects/wireless-domain.png"));
      images.put(AbstractObject.OBJECT_ZONE, ResourceManager.getImage(display, "icons/objects/zone.gif"));

      display.disposeExec(() -> {
         for(Image i : images.values())
            i.dispose();
      });
   }

   /**
    * Get icon image for given object class.
    *
    * @param objectClass object class
    * @return icon image for given object class
    */
   public Image getImage(int objectClass)
   {
      Image image = images.get(objectClass);
      return (image != null) ? image : SharedIcons.IMG_UNKNOWN_OBJECT;
   }
}
