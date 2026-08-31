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

      Label title = new Label(getLinkTitle());
      title.setFont(JFaceResources.getBannerFont());
      add(title);

      ObjectStatus status = calculateLinkStatus();
      Label statusLabel = new Label();
      statusLabel.setIcon(StatusDisplayInfo.getStatusImage(status));
      statusLabel.setText(StatusDisplayInfo.getStatusText(status));
      add(statusLabel);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      setConstraint(statusLabel, gd);

      Label type = new Label(i18n.tr("Type: {0}", getLinkTypeName()));
      add(type);
      gd = new GridData();
      gd.horizontalSpan = 2;
      setConstraint(type, gd);

      addEndpoint(link.getElement1(), link.getInterfaceId1(), link.getConnectorName1());
      addEndpoint(link.getElement2(), link.getInterfaceId2(), link.getConnectorName2());
   }

   /**
    * Add information block for single link endpoint.
    *
    * @param elementId map element ID
    * @param interfaceId interface object ID on this side of the link (0 if unknown)
    * @param connectorName connector name configured for this side of the link
    */
   private void addEndpoint(long elementId, long interfaceId, String connectorName)
   {
      AbstractObject object = resolveElementObject(elementId);
      AbstractObject interfaceObject = (interfaceId != 0) ? session.findObjectById(interfaceId, true) : null;
      Interface iface = (interfaceObject instanceof Interface) ? (Interface)interfaceObject : null;

      addSeparator();

      Label objectLabel = new Label();
      objectLabel.setText((object != null) ? object.getObjectName() : i18n.tr("<unknown>"));
      if (object != null)
         objectLabel.setIcon(labelProvider.getSmallIcon(object));
      add(objectLabel);

      if (object != null)
      {
         Label objectStatus = new Label();
         objectStatus.setIcon(StatusDisplayInfo.getStatusImage(object.getStatus()));
         objectStatus.setText(StatusDisplayInfo.getStatusText(object.getStatus()));
         add(objectStatus);
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.RIGHT;
         setConstraint(objectStatus, gd);
      }
      else
      {
         add(new Label());
      }

      Figure details = new Figure();
      GridLayout layout = new GridLayout(2, false);
      layout.horizontalSpacing = 10;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      details.setLayoutManager(layout);

      if (iface != null)
      {
         addDetail(details, i18n.tr("Interface"), iface.getNameWithAlias());
         addDetail(details, i18n.tr("Oper state"), iface.getOperStateAsText());
         addDetail(details, i18n.tr("Admin state"), iface.getAdminStateAsText());
         if (iface.getIfType() != 0)
         {
            String typeName = iface.getIfTypeName();
            addDetail(details, i18n.tr("Type"),
                  (typeName != null) ? String.format("%d (%s)", iface.getIfType(), typeName) : Integer.toString(iface.getIfType()));
         }
         if (iface.getSpeed() > 0)
            addDetail(details, i18n.tr("Speed"), InterfaceListLabelProvider.ifSpeedTotext(iface.getSpeed()));
         addUtilizationDetail(details, i18n.tr("Inbound"), iface.getInboundUtilization(), iface.getSpeed());
         addUtilizationDetail(details, i18n.tr("Outbound"), iface.getOutboundUtilization(), iface.getSpeed());
      }
      else if ((connectorName != null) && !connectorName.isBlank())
      {
         addDetail(details, i18n.tr("Connector"), connectorName);
      }

      if (!details.getChildren().isEmpty())
      {
         GridData gd = new GridData();
         gd.horizontalSpan = 2;
         gd.horizontalIndent = 16;
         add(details, gd);
      }
   }

   /**
    * Add name/value pair to endpoint details block.
    *
    * @param parent details block figure
    * @param name value name
    * @param value value to display
    */
   private void addDetail(Figure parent, String name, String value)
   {
      if ((value == null) || value.isEmpty())
         return;

      parent.add(new Label(name + ":"));
      parent.add(new Label(value));
   }

   /**
    * Add utilization name/value pair to endpoint details block. If interface speed is known, actual traffic calculated from
    * utilization and speed is shown as well.
    *
    * @param parent details block figure
    * @param name value name
    * @param utilization utilization in permille or -1 if not available
    * @param speed interface speed in bps or 0 if not known
    */
   private void addUtilizationDetail(Figure parent, String name, int utilization, long speed)
   {
      if (utilization < 0)
         return;

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
      addDetail(parent, name, sb.toString());
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
    * Get title for tooltip - link name if set, otherwise generic text.
    *
    * @return tooltip title
    */
   private String getLinkTitle()
   {
      String name = link.getName();
      return ((name != null) && !name.isBlank()) ? name : i18n.tr("Link");
   }

   /**
    * Get display name for link type.
    *
    * @return display name for link type
    */
   private String getLinkTypeName()
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
    * @return calculated link status
    */
   private ObjectStatus calculateLinkStatus()
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
         status = mostCritical(status, getInterfaceStatus(link.getInterfaceId1()));
         status = mostCritical(status, getInterfaceStatus(link.getInterfaceId2()));
      }

      return status;
   }

   /**
    * Get status of interface object with given ID.
    *
    * @param interfaceId interface object ID (0 if unknown)
    * @return interface status or UNKNOWN if interface object is not available
    */
   private ObjectStatus getInterfaceStatus(long interfaceId)
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
    * @param elementId map element ID
    * @return object represented by given map element or null
    */
   private AbstractObject resolveElementObject(long elementId)
   {
      Object input = labelProvider.getViewer().getInput();
      if (!(input instanceof NetworkMapPage))
         return null;

      NetworkMapElement element = ((NetworkMapPage)input).getElement(elementId, NetworkMapObject.class);
      return (element != null) ? session.findObjectById(((NetworkMapObject)element).getObjectId(), true) : null;
   }
}
