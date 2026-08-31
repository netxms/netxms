/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.GridData;
import org.eclipse.draw2d.GridLayout;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.MarginBorder;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.RGB;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.InterfaceListLabelProvider;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * Tooltip for link on map. Shows link type and status, and for each of the two endpoints - object, interface, interface speed and
 * type, and current utilization and traffic.
 */
public class LinkTooltip extends Figure
{
   private static final RGB SEPARATOR_COLOR = new RGB(128, 128, 128);

   private final I18n i18n = LocalizationHelper.getI18n(LinkTooltip.class);

   private NXCSession session = Registry.getSession();
   private NetworkMapLink link;
   private MapLabelProvider labelProvider;

   /**
    * Create tooltip for given map link.
    *
    * @param link map link
    * @param labelProvider map label provider
    */
   public LinkTooltip(NetworkMapLink link, MapLabelProvider labelProvider)
   {
      this.link = link;
      this.labelProvider = labelProvider;

      setBorder(new MarginBorder(3));
      GridLayout layout = new GridLayout(2, false);
      layout.horizontalSpacing = 10;
      setLayoutManager(layout);

      setOpaque(true);
      setBackgroundColor(ThemeEngine.getBackgroundColor("Map.ObjectTooltip"));

      buildContent();
   }

   /**
    * Tooltip content is rebuilt each time tooltip figure is attached to tooltip shell (which happens right before it is shown), so
    * displayed interface state, utilization, and traffic always reflect current object cache content.
    *
    * @see org.eclipse.draw2d.Figure#addNotify()
    */
   @Override
   public void addNotify()
   {
      super.addNotify();
      buildContent();
   }

   /**
    * Build tooltip content from current state of referenced objects.
    */
   private void buildContent()
   {
      removeAll();

      Label title = new Label(getLinkTitle(i18n, link));
      title.setFont(JFaceResources.getBannerFont());
      add(title);

      ObjectStatus status = calculateLinkStatus(session, link);
      Label statusLabel = new Label(StatusDisplayInfo.getStatusText(status));
      statusLabel.setIcon(StatusDisplayInfo.getStatusImage(status));
      add(statusLabel);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      setConstraint(statusLabel, gd);

      Label type = new Label(i18n.tr("Type: {0}", getLinkTypeName(i18n, link)));
      add(type);
      gd = new GridData();
      gd.horizontalSpan = 2;
      setConstraint(type, gd);

      Object input = labelProvider.getViewer().getInput();
      NetworkMapPage page = (input instanceof NetworkMapPage) ? (NetworkMapPage)input : null;
      for(Endpoint endpoint : collectEndpoints(i18n, session, link, page))
      {
         addSeparator();

         Label objectLabel = new Label(endpoint.name);
         if (endpoint.object != null)
            objectLabel.setIcon(labelProvider.getSmallIcon(endpoint.object));
         add(objectLabel);

         if (endpoint.object != null)
         {
            Label objectStatus = new Label(StatusDisplayInfo.getStatusText(endpoint.object.getStatus()));
            objectStatus.setIcon(StatusDisplayInfo.getStatusImage(endpoint.object.getStatus()));
            add(objectStatus);
            gd = new GridData();
            gd.horizontalAlignment = SWT.RIGHT;
            setConstraint(objectStatus, gd);
         }
         else
         {
            add(new Label());
         }

         if (endpoint.details.isEmpty())
            continue;

         Figure details = new Figure();
         GridLayout detailsLayout = new GridLayout(2, false);
         detailsLayout.horizontalSpacing = 10;
         detailsLayout.marginWidth = 0;
         detailsLayout.marginHeight = 0;
         details.setLayoutManager(detailsLayout);
         for(String[] detail : endpoint.details)
         {
            details.add(new Label(detail[0] + ":"));
            details.add(new Label(detail[1]));
         }

         gd = new GridData();
         gd.horizontalSpan = 2;
         gd.horizontalIndent = 16;
         add(details, gd);
      }
   }

   /**
    * Add horizontal separator spanning both tooltip columns.
    */
   private void addSeparator()
   {
      Figure separator = new Figure();
      separator.setOpaque(true);
      separator.setBackgroundColor(labelProvider.getColors().create(SEPARATOR_COLOR));

      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 1;
      add(separator, gd);
   }

   /**
    * Build plain text description of link with same content as this tooltip. Intended for map canvases that cannot host draw2d
    * figures and have to use native tooltips.
    *
    * @param link map link
    * @param page map page containing the link (can be null)
    * @return plain text description of the link
    */
   public static String buildDescription(NetworkMapLink link, NetworkMapPage page)
   {
      I18n i18n = LocalizationHelper.getI18n(LinkTooltip.class);
      NXCSession session = Registry.getSession();

      StringBuilder sb = new StringBuilder(getLinkTitle(i18n, link));
      sb.append('\n').append(StatusDisplayInfo.getStatusText(calculateLinkStatus(session, link)));
      sb.append('\n').append(i18n.tr("Type: {0}", getLinkTypeName(i18n, link)));

      for(Endpoint endpoint : collectEndpoints(i18n, session, link, page))
      {
         sb.append('\n').append(endpoint.name);
         if (endpoint.object != null)
            sb.append(" (").append(StatusDisplayInfo.getStatusText(endpoint.object.getStatus())).append(')');
         for(String[] detail : endpoint.details)
            sb.append("\n\t").append(detail[0]).append(": ").append(detail[1]);
      }

      return sb.toString();
   }

   /**
    * Collect information about both endpoints of the link.
    *
    * @param i18n localization object
    * @param session client session
    * @param link map link
    * @param page map page containing the link (can be null)
    * @return information about both endpoints of the link
    */
   private static List<Endpoint> collectEndpoints(I18n i18n, NXCSession session, NetworkMapLink link, NetworkMapPage page)
   {
      List<Endpoint> endpoints = new ArrayList<Endpoint>(2);
      endpoints.add(collectEndpoint(i18n, session, page, link.getElement1(), link.getInterfaceId1(), link.getConnectorName1()));
      endpoints.add(collectEndpoint(i18n, session, page, link.getElement2(), link.getInterfaceId2(), link.getConnectorName2()));
      return endpoints;
   }

   /**
    * Collect information about single link endpoint.
    *
    * @param i18n localization object
    * @param session client session
    * @param page map page containing the link (can be null)
    * @param elementId map element ID
    * @param interfaceId interface object ID on this side of the link (0 if unknown)
    * @param connectorName connector name configured for this side of the link
    * @return information about link endpoint
    */
   private static Endpoint collectEndpoint(I18n i18n, NXCSession session, NetworkMapPage page, long elementId, long interfaceId,
         String connectorName)
   {
      Endpoint endpoint = new Endpoint();
      endpoint.object = resolveElementObject(session, page, elementId);
      endpoint.name = (endpoint.object != null) ? endpoint.object.getObjectName() : i18n.tr("<unknown>");

      AbstractObject interfaceObject = (interfaceId != 0) ? session.findObjectById(interfaceId, true) : null;
      if (interfaceObject instanceof Interface)
      {
         Interface iface = (Interface)interfaceObject;
         endpoint.addDetail(i18n.tr("Interface"), iface.getNameWithAlias());
         endpoint.addDetail(i18n.tr("Oper state"), iface.getOperStateAsText());
         endpoint.addDetail(i18n.tr("Admin state"), iface.getAdminStateAsText());
         if (iface.getIfType() != 0)
         {
            String typeName = iface.getIfTypeName();
            endpoint.addDetail(i18n.tr("Type"),
                  (typeName != null) ? String.format("%d (%s)", iface.getIfType(), typeName) : Integer.toString(iface.getIfType()));
         }
         if (iface.getSpeed() > 0)
            endpoint.addDetail(i18n.tr("Speed"), InterfaceListLabelProvider.ifSpeedTotext(iface.getSpeed()));
         endpoint.addDetail(i18n.tr("Inbound"), formatUtilization(iface.getInboundUtilization(), iface.getSpeed()));
         endpoint.addDetail(i18n.tr("Outbound"), formatUtilization(iface.getOutboundUtilization(), iface.getSpeed()));
      }
      else if ((connectorName != null) && !connectorName.isBlank())
      {
         endpoint.addDetail(i18n.tr("Connector"), connectorName);
      }

      return endpoint;
   }

   /**
    * Format interface utilization. If interface speed is known, actual traffic calculated from utilization and speed is added.
    *
    * @param utilization utilization in permille or -1 if not available
    * @param speed interface speed in bps or 0 if not known
    * @return formatted utilization or null if utilization is not available
    */
   private static String formatUtilization(int utilization, long speed)
   {
      if (utilization < 0)
         return null;

      StringBuilder sb = new StringBuilder();
      sb.append(utilization / 10);
      sb.append('.');
      sb.append(utilization % 10);
      sb.append('%');
      if (speed > 0)
      {
         sb.append(" (");
         sb.append(InterfaceListLabelProvider.ifSpeedTotext(speed * utilization / 1000));
         sb.append(')');
      }
      return sb.toString();
   }

   /**
    * Get title for tooltip - link name if set, otherwise generic text.
    *
    * @param i18n localization object
    * @param link map link
    * @return tooltip title
    */
   private static String getLinkTitle(I18n i18n, NetworkMapLink link)
   {
      String name = link.getName();
      return ((name != null) && !name.isBlank()) ? name : i18n.tr("Link");
   }

   /**
    * Get display name for link type.
    *
    * @param i18n localization object
    * @param link map link
    * @return display name for link type
    */
   private static String getLinkTypeName(I18n i18n, NetworkMapLink link)
   {
      switch(link.getType())
      {
         case NetworkMapLink.NORMAL:
            return i18n.tr("Normal");
         case NetworkMapLink.VPN:
            return i18n.tr("VPN");
         case NetworkMapLink.MULTILINK:
            return i18n.tr("Multiple links");
         case NetworkMapLink.AGENT_TUNEL:
            return i18n.tr("Agent tunnel");
         case NetworkMapLink.AGENT_PROXY:
            return i18n.tr("Agent proxy");
         case NetworkMapLink.SSH_PROXY:
            return i18n.tr("SSH proxy");
         case NetworkMapLink.SNMP_PROXY:
            return i18n.tr("SNMP proxy");
         case NetworkMapLink.ICMP_PROXY:
            return i18n.tr("ICMP proxy");
         case NetworkMapLink.SENSOR_PROXY:
            return i18n.tr("Sensor proxy");
         case NetworkMapLink.ZONE_PROXY:
            return i18n.tr("Zone proxy");
         case NetworkMapLink.WIFI_CLIENT:
            return i18n.tr("Wireless client");
         default:
            return Integer.toString(link.getType());
      }
   }

   /**
    * Calculate link status as most critical status of link's status objects, or, if link has no status objects, of interfaces on
    * both sides of the link.
    *
    * @param session client session
    * @param link map link
    * @return calculated link status
    */
   private static ObjectStatus calculateLinkStatus(NXCSession session, NetworkMapLink link)
   {
      ObjectStatus status = ObjectStatus.UNKNOWN;
      for(Long id : link.getStatusObjects())
      {
         AbstractObject object = session.findObjectById(id, true);
         if (object != null)
            status = mostCritical(status, object.getStatus());
      }

      if (status == ObjectStatus.UNKNOWN)
      {
         status = mostCritical(status, getInterfaceStatus(session, link.getInterfaceId1()));
         status = mostCritical(status, getInterfaceStatus(session, link.getInterfaceId2()));
      }

      return status;
   }

   /**
    * Get status of interface object with given ID.
    *
    * @param session client session
    * @param interfaceId interface object ID (0 if unknown)
    * @return interface status or UNKNOWN if interface object is not available
    */
   private static ObjectStatus getInterfaceStatus(NXCSession session, long interfaceId)
   {
      AbstractObject object = (interfaceId != 0) ? session.findObjectById(interfaceId, true) : null;
      return (object instanceof Interface) ? object.getStatus() : ObjectStatus.UNKNOWN;
   }

   /**
    * Select most critical of two statuses, ignoring statuses that do not indicate a problem level (unknown, unmanaged, etc.).
    *
    * @param current currently selected status
    * @param candidate status to check
    * @return most critical of two statuses
    */
   private static ObjectStatus mostCritical(ObjectStatus current, ObjectStatus candidate)
   {
      if (candidate.compareTo(ObjectStatus.UNKNOWN) >= 0)
         return current;
      return ((current == ObjectStatus.UNKNOWN) || (candidate.compareTo(current) > 0)) ? candidate : current;
   }

   /**
    * Find object represented by given map element.
    *
    * @param session client session
    * @param page map page containing the element (can be null)
    * @param elementId map element ID
    * @return object represented by given map element or null
    */
   private static AbstractObject resolveElementObject(NXCSession session, NetworkMapPage page, long elementId)
   {
      if (page == null)
         return null;

      NetworkMapElement element = page.getElement(elementId, NetworkMapObject.class);
      return (element != null) ? session.findObjectById(((NetworkMapObject)element).getObjectId(), true) : null;
   }

   /**
    * Information about single link endpoint collected for display.
    */
   private static final class Endpoint
   {
      AbstractObject object;
      String name;
      List<String[]> details = new ArrayList<String[]>();

      /**
       * Add name/value pair to endpoint details. Pairs with empty values are ignored.
       *
       * @param name value name
       * @param value value
       */
      void addDetail(String name, String value)
      {
         if ((value != null) && !value.isEmpty())
            details.add(new String[] { name, value });
      }
   }
}
