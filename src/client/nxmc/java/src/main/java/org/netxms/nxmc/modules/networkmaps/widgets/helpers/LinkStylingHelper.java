/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import java.util.List;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Resolution of color, width and style for a network map link, shared between
 * the Zest-based graph canvas ({@link MapLabelProvider}) and the geographical
 * canvas renderer. The logic here is coordinate-system agnostic and must not
 * depend on a {@code GraphConnection} or any other Zest type.
 */
public final class LinkStylingHelper
{
   private static final Logger logger = LoggerFactory.getLogger(LinkStylingHelper.class);

   public static final Color COLOR_AGENT_TUNNEL = new Color(Display.getDefault(), new RGB(255, 0, 0));
   public static final Color COLOR_AGENT_PROXY  = new Color(Display.getDefault(), new RGB(0, 255, 0));
   public static final Color COLOR_ICMP_PROXY   = new Color(Display.getDefault(), new RGB(0, 0, 255));
   public static final Color COLOR_SNMP_PROXY   = new Color(Display.getDefault(), new RGB(255, 255, 0));
   public static final Color COLOR_SSH_PROXY    = new Color(Display.getDefault(), new RGB(0, 255, 255));
   public static final Color COLOR_ZONE_PROXY   = new Color(Display.getDefault(), new RGB(255, 0, 255));

   public static final Color[] COLOR_LINK_UTILIZATION = {
         new Color(Display.getDefault(), new RGB(192, 192, 192)), // 0%
         new Color(Display.getDefault(), new RGB(118, 48, 250)),  // 1-9%
         new Color(Display.getDefault(), new RGB(0, 57, 250)),    // 10-24%
         new Color(Display.getDefault(), new RGB(25, 188, 252)),  // 25-39%
         new Color(Display.getDefault(), new RGB(64, 233, 45)),   // 40-54%
         new Color(Display.getDefault(), new RGB(241, 233, 48)),  // 55-69%
         new Color(Display.getDefault(), new RGB(254, 180, 39)),  // 70-84%
         new Color(Display.getDefault(), new RGB(249, 0, 16)),    // 85-100%
   };
   public static final int[] LINK_UTILIZATION_THRESHOLDS = { 10, 100, 250, 400, 550, 700, 850, 1000 };

   private LinkStylingHelper()
   {
   }

   /**
    * Resolve the line color for a link given its own configuration and the
    * map's default color source/color. Returns {@code null} if neither source
    * nor default yields a color, in which case the caller should leave the
    * connection's color untouched.
    */
   public static Color resolveLinkColor(NetworkMapLink link, NXCSession session,
         LinkDciValueProvider dciValueProvider, ColorCache colors,
         Color defaultLinkColor, int defaultLinkColorSource)
   {
      int source = link.getColorSource();

      if (source == NetworkMapLink.COLOR_SOURCE_OBJECT_STATUS)
         return resolveObjectStatusColor(link, session, dciValueProvider);

      if (source == NetworkMapLink.COLOR_SOURCE_INTERFACE_STATUS)
         return resolveInterfaceStatusColor(link, session);

      if (source == NetworkMapLink.COLOR_SOURCE_LINK_UTILIZATION)
         return resolveLinkUtilizationColor(link, session);

      if ((source == NetworkMapLink.COLOR_SOURCE_CUSTOM_COLOR) || (source == NetworkMapLink.COLOR_SOURCE_SCRIPT))
         return colors.create(ColorConverter.rgbFromInt(link.getColor()));

      // COLOR_SOURCE_DEFAULT - inherit from map
      if (defaultLinkColorSource == NetworkMapLink.COLOR_SOURCE_INTERFACE_STATUS)
         return resolveInterfaceStatusColor(link, session);
      if (defaultLinkColorSource == NetworkMapLink.COLOR_SOURCE_LINK_UTILIZATION)
         return resolveLinkUtilizationColor(link, session);

      switch(link.getType())
      {
         case NetworkMapLink.AGENT_TUNEL: return COLOR_AGENT_TUNNEL;
         case NetworkMapLink.AGENT_PROXY: return COLOR_AGENT_PROXY;
         case NetworkMapLink.ICMP_PROXY:  return COLOR_ICMP_PROXY;
         case NetworkMapLink.SNMP_PROXY:  return COLOR_SNMP_PROXY;
         case NetworkMapLink.SSH_PROXY:   return COLOR_SSH_PROXY;
         case NetworkMapLink.ZONE_PROXY:  return COLOR_ZONE_PROXY;
      }

      return defaultLinkColor;
   }

   /**
    * Resolve line width: use the link's configured width when non-zero,
    * otherwise fall back to the map default.
    */
   public static int resolveLinkWidth(NetworkMapLink link, int defaultLinkWidth)
   {
      int w = link.getConfig().getWidth();
      return (w == 0) ? defaultLinkWidth : w;
   }

   /**
    * Resolve line style: valid range is 1..5, anything else means "use default".
    */
   public static int resolveLinkStyle(NetworkMapLink link, int defaultLinkStyle)
   {
      int s = link.getConfig().getStyle();
      return (s == 0 || s > 5) ? defaultLinkStyle : s;
   }

   private static Color resolveObjectStatusColor(NetworkMapLink link, NXCSession session, LinkDciValueProvider dciValueProvider)
   {
      ObjectStatus status = ObjectStatus.UNKNOWN;
      ObjectStatus altStatus = ObjectStatus.UNKNOWN;
      int interfaceUtilization = 0;
      for(Long id : link.getStatusObjects())
      {
         AbstractObject object = session.findObjectById(id, true);
         if (object == null)
            continue;

         ObjectStatus s = object.getStatus();
         if ((s.compareTo(ObjectStatus.UNKNOWN) < 0) && ((status.compareTo(s) < 0) || (status == ObjectStatus.UNKNOWN)))
         {
            status = s;
            if (status == ObjectStatus.CRITICAL)
               break;
         }
         if (s != ObjectStatus.UNKNOWN)
            altStatus = s;
         if (object instanceof Interface)
            interfaceUtilization = Math.max(interfaceUtilization, ((Interface)object).getOutboundUtilization());
      }

      if (link.getConfig().isUseActiveThresholds() && !link.getDciAsList().isEmpty())
      {
         Severity severity = Severity.UNKNOWN;
         try
         {
            List<DciValue> values = dciValueProvider.getDciData(link.getDciAsList());
            for(DciValue v : values)
            {
               Severity s = v.getMostCriticalSeverity();
               if ((s.compareTo(Severity.UNKNOWN) < 0) && ((severity.compareTo(s) < 0) || (severity == Severity.UNKNOWN)))
               {
                  severity = s;
                  if (severity == Severity.CRITICAL)
                     break;
               }
            }
         }
         catch(Exception e)
         {
            logger.error("Exception in link styling helper", e);
         }
         if ((severity != Severity.UNKNOWN) && ((severity.getValue() > status.getValue()) || (status.compareTo(ObjectStatus.UNKNOWN) >= 0)))
            status = ObjectStatus.getByValue(severity.getValue());
      }

      if (link.getConfig().isUseInterfaceUtilization() && (status != ObjectStatus.CRITICAL) && (altStatus != ObjectStatus.DISABLED))
      {
         int index = 0;
         while((interfaceUtilization >= LINK_UTILIZATION_THRESHOLDS[index]) && (++index < LINK_UTILIZATION_THRESHOLDS.length - 1))
            ;
         return COLOR_LINK_UTILIZATION[index];
      }
      return StatusDisplayInfo.getStatusColor((status != ObjectStatus.UNKNOWN) ? status : altStatus);
   }

   private static Color resolveInterfaceStatusColor(NetworkMapLink link, NXCSession session)
   {
      ObjectStatus status = ObjectStatus.UNKNOWN;
      AbstractObject object = session.findObjectById(link.getInterfaceId1(), true);
      if (object instanceof Interface)
         status = object.getStatus();
      object = session.findObjectById(link.getInterfaceId2(), true);
      if ((object instanceof Interface) && (object.getStatus() != ObjectStatus.UNKNOWN))
      {
         if ((status == ObjectStatus.UNKNOWN) || (object.getStatus().getValue() > status.getValue()))
            status = object.getStatus();
      }
      return StatusDisplayInfo.getStatusColor(status);
   }

   private static Color resolveLinkUtilizationColor(NetworkMapLink link, NXCSession session)
   {
      int interfaceUtilization = 0;
      AbstractObject object = session.findObjectById(link.getInterfaceId1(), true);
      if (object instanceof Interface)
         interfaceUtilization = Math.max(interfaceUtilization, ((Interface)object).getOutboundUtilization());
      object = session.findObjectById(link.getInterfaceId2(), true);
      if (object instanceof Interface)
         interfaceUtilization = Math.max(interfaceUtilization, ((Interface)object).getOutboundUtilization());

      int index = 0;
      while((interfaceUtilization >= LINK_UTILIZATION_THRESHOLDS[index]) && (++index < LINK_UTILIZATION_THRESHOLDS.length - 1))
         ;
      return COLOR_LINK_UTILIZATION[index];
   }
}
