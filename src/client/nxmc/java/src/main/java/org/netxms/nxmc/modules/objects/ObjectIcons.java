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
import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.imagelibrary.ImageProviderTools;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Storage for object icons. Holds images created on owning display, so each display has own instance.
 */
public final class ObjectIcons
{
   private static final Logger logger = LoggerFactory.getLogger(ObjectIcons.class);

   /**
    * Initialize object icon storage
    * 
    * @param display curent display
    */
   public static void init(Display display)
   {
      Registry.setDisplaySingleton(display, ObjectIcons.class, new ObjectIcons(display));
   }

   private Map<Integer, Image> images;
   private Image nodeSwitch;
   private Image nodeRouter;
   private Image nodePrinter;
   private Image nodeWindows;
   private Image nodeLinux;
   private Image nodeFreeBSD;

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
      images.put(AbstractObject.OBJECT_COOLINGZONE, ResourceManager.getImage(display, "icons/objects/cooling-zone.png"));
      images.put(AbstractObject.OBJECT_ROOM, ResourceManager.getImage(display, "icons/objects/room.png"));
      images.put(AbstractObject.OBJECT_FACILITY, ResourceManager.getImage(display, "icons/objects/facility.png"));
      images.put(AbstractObject.OBJECT_POWERDOMAIN, ResourceManager.getImage(display, "icons/objects/power-domain.png"));
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

      nodeSwitch = createNodeIcon(display, "icons/netmap/objects/switch.png");
      nodeRouter = createNodeIcon(display, "icons/netmap/objects/router.png");
      nodePrinter = createNodeIcon(display, "icons/netmap/objects/printer.png");
      nodeWindows = createNodeIcon(display, "icons/netmap/objects/windowsserver.png");
      nodeLinux = createNodeIcon(display, "icons/netmap/objects/linuxserver.png");
      nodeFreeBSD = createNodeIcon(display, "icons/netmap/objects/freebsdserver.png");
      images.put(AbstractObject.OBJECT_RACK, ResourceManager.getImage(display, "icons/objects/rack.gif"));
      images.put(AbstractObject.OBJECT_SENSOR, ResourceManager.getImage(display, "icons/objects/sensor.gif"));
      images.put(AbstractObject.OBJECT_SERVICEROOT, ResourceManager.getImage(display, "icons/objects/service_root.png"));
      images.put(AbstractObject.OBJECT_SUBNET, ResourceManager.getImage(display, "icons/objects/subnet.png"));
      images.put(AbstractObject.OBJECT_TEMPLATE, ResourceManager.getImage(display, "icons/objects/template.png"));
      images.put(AbstractObject.OBJECT_TEMPLATEGROUP, ResourceManager.getImage(display, "icons/objects/template_group.png"));
      images.put(AbstractObject.OBJECT_TEMPLATEROOT, ResourceManager.getImage(display, "icons/objects/template_root.png"));
      images.put(AbstractObject.OBJECT_VPNCONNECTOR, ResourceManager.getImage(display, "icons/objects/vpn.png"));
      images.put(AbstractObject.OBJECT_WIRELESSDOMAIN, ResourceManager.getImage(display, "icons/objects/wireless-domain.png"));
      images.put(AbstractObject.OBJECT_CLOUDDOMAIN, ResourceManager.getImage(display, "icons/objects/cloud-domain.png"));
      images.put(AbstractObject.OBJECT_RESOURCE, ResourceManager.getImage(display, "icons/objects/resource.png"));
      images.put(AbstractObject.OBJECT_TRAFFICOBSERVER, ResourceManager.getImage(display, "icons/objects/traffic-observer.png"));
      images.put(AbstractObject.OBJECT_OBSERVATIONPOINT, ResourceManager.getImage(display, "icons/objects/observation-point.png"));
      images.put(AbstractObject.OBJECT_ZONE, ResourceManager.getImage(display, "icons/objects/zone.gif"));

      display.disposeExec(() -> {
         for(Image i : images.values())
            i.dispose();
         disposeImage(nodeSwitch);
         disposeImage(nodeRouter);
         disposeImage(nodePrinter);
         disposeImage(nodeWindows);
         disposeImage(nodeLinux);
         disposeImage(nodeFreeBSD);
      });
   }

   /**
    * Create a 16x16 object-browser icon from an existing network map image.
    *
    * @param display current display
    * @param path image resource path
    * @return scaled icon image, or null if the image cannot be loaded or scaled
    */
   private static Image createNodeIcon(Display display, String path)
   {
      Image source = null;
      Image icon = null;
      try
      {
         // createImage(false, ...) returns null instead of a placeholder image when the resource is missing,
         // so a missing icon falls back to the generic node icon
         source = ResourceManager.getImageDescriptor(path).createImage(false, display);
         if (source == null)
         {
            logger.warn("Cannot load node icon image from {}", path);
            return null;
         }
         icon = ImageProviderTools.createResizedImage(source, 16);
      }
      catch(SWTException e)
      {
         logger.warn("Cannot load or create node icon image from {}", path, e);
      }
      finally
      {
         if ((source != null) && (source != icon))
            source.dispose();
      }
      return icon;
   }

   /**
    * Dispose image if it was successfully loaded.
    *
    * @param image image to dispose
    */
   private static void disposeImage(Image image)
   {
      if (image != null)
         image.dispose();
   }

   /**
    * Return a specialized node icon, or the generic node icon if loading failed.
    *
    * @param image specialized node icon
    * @return specialized node icon or generic node icon
    */
   private Image getNodeIconOrDefault(Image image)
   {
      return (image != null) ? image : getImage(AbstractObject.OBJECT_NODE);
   }

   /**
    * Get icon image for a node using its discovered capabilities and platform.
    *
    * @param node node object
    * @return icon image for node
    */
   public Image getImage(Node node)
   {
      long capabilities = node.getCapabilities();
      if ((capabilities & Node.NC_IS_BRIDGE) != 0)
         return getNodeIconOrDefault(nodeSwitch);
      if ((capabilities & Node.NC_IS_ROUTER) != 0)
         return getNodeIconOrDefault(nodeRouter);
      if ((capabilities & Node.NC_IS_PRINTER) != 0)
         return getNodeIconOrDefault(nodePrinter);

      String platformName = node.getPlatformName();
      if (platformName != null)
      {
         if (platformName.startsWith("windows"))
            return getNodeIconOrDefault(nodeWindows);
         if (platformName.startsWith("Linux"))
            return getNodeIconOrDefault(nodeLinux);
         if (platformName.startsWith("FreeBSD"))
            return getNodeIconOrDefault(nodeFreeBSD);
      }

      return getImage(AbstractObject.OBJECT_NODE);
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
