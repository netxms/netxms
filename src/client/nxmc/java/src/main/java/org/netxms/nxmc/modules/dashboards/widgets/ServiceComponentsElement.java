/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.util.Iterator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Condition;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.dashboards.config.ServiceComponentsConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.networkmaps.widgets.NetworkMapWidget;
import org.netxms.nxmc.tools.ColorConverter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * Service components map element for dashboard
 */
public class ServiceComponentsElement extends ElementWidget
{
   private static final Logger logger = LoggerFactory.getLogger(ServiceComponentsElement.class);
   
   private NetworkMapWidget mapWidget;
   private NetworkMapPage mapPage;
   private ServiceComponentsConfig config;
   private NXCSession session;

   /**
    * Create new service components map element
    * 
    * @param parent Dashboard control
    * @param element Dashboard element
    * @param view owning view
    */
   protected ServiceComponentsElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);

      try
      {
         config = new Gson().fromJson(element.getData(), ServiceComponentsConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         config = new ServiceComponentsConfig();
      }

      processCommonSettings(config);

      session = Registry.getSession();
      long rootObjectId = getEffectiveObjectId(config.getObjectId());

      mapPage = new NetworkMapPage("ServiceComponents." + rootObjectId);
      long elementId = mapPage.createElementId();
      mapPage.addElement(new NetworkMapObject(elementId, rootObjectId));
      addServiceComponents(session.findObjectById(rootObjectId), elementId);

      if (mapPage != null)
      {
         mapWidget = new NetworkMapWidget(getContentArea(), SWT.NONE, view);
         mapWidget.setLayoutAlgorithm(config.getDefaultLayoutAlgorithm());
         mapWidget.zoomTo((double)config.getZoomLevel() / 100.0);
         mapWidget.getLabelProvider().setObjectFigureType(config.getObjectDisplayMode());
         mapWidget.getLabelProvider().setShowStatusIcons((config.getFlags() & NetworkMap.MF_SHOW_STATUS_ICON) != 0);
         mapWidget.getLabelProvider().setShowStatusFrame((config.getFlags() & NetworkMap.MF_SHOW_STATUS_FRAME) != 0);
         mapWidget.getLabelProvider().setShowStatusBackground((config.getFlags() & NetworkMap.MF_SHOW_STATUS_BKGND) != 0);
         mapWidget.getLabelProvider().setTranslucentLabelBackground((config.getFlags() & NetworkMap.MF_TRANSLUCENT_LABEL_BKGND) != 0);
         mapWidget.setConnectionRouter(config.getDefaultLinkRouting());
         if (config.getDefaultLinkColor() >= 0)
            mapWidget.getLabelProvider().setDefaultLinkColor(new Color(mapWidget.getControl().getDisplay(), ColorConverter.rgbFromInt(config.getDefaultLinkColor())));
         mapWidget.setContent(mapPage);
      }

      if (config.isObjectDoubleClickEnabled())
      {
         mapWidget.enableObjectDoubleClick();
      }
   }
   
   /**
    * Add parent services for given object
    * 
    * @param object
    */
   private void addServiceComponents(AbstractObject object, long parentElementId)
   {
      if (object == null)
         return;

      Iterator<Long> it = object.getChildren();
      while(it.hasNext())
      {
         long objectId = it.next();
         AbstractObject child = session.findObjectById(objectId);
         if ((child != null) && 
               ((child instanceof Collector) ||
                (child instanceof Container) || 
                (child instanceof Cluster) || 
                (child instanceof Node) ||
                (child instanceof Condition)))
         {
            long elementId = mapPage.createElementId();
            mapPage.addElement(new NetworkMapObject(elementId, objectId));
            mapPage.addLink(new NetworkMapLink(mapPage.createLinkId(), NetworkMapLink.NORMAL, parentElementId, elementId));
            addServiceComponents(child, elementId);
         }
      }
   }
}
