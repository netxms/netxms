/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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

import org.netxms.client.NXCSession;
import org.netxms.client.maps.LinkDataLocation;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Text of network map link labels, shared by the graph canvas and the geographical canvas. Each label location (connector
 * name at either end, OBJECT1/OBJECT2 values, center) is built separately; for a {@link ParallelLinkGroup} every location
 * lists its members, one row per member.
 */
public final class LinkLabelText
{
   private static final I18n i18n = LocalizationHelper.getI18n(LinkLabelText.class);

   private LinkLabelText()
   {
   }

   /**
    * Get connector name for one end of the link: explicit connector name if set, otherwise name of the interface object on
    * that end. For a group: names of all members on that end, comma-separated.
    *
    * @param link map link
    * @param second true for element2 end
    * @param session client session
    * @return connector name or null if neither name nor interface is known
    */
   public static String connectorName(NetworkMapLink link, boolean second, NXCSession session)
   {
      if (link instanceof ParallelLinkGroup)
      {
         ParallelLinkGroup group = (ParallelLinkGroup)link;
         StringBuilder sb = new StringBuilder();
         for(NetworkMapLink member : group.getLinks())
         {
            String name = connectorName(member, second != group.isInverted(member), session);
            if (name == null)
               continue;
            if (sb.length() > 0)
               sb.append(", ");
            sb.append(name);
         }
         return (sb.length() > 0) ? sb.toString() : null;
      }

      String name = second ? link.getConnectorName2() : link.getConnectorName1();
      if ((name != null) && !name.isBlank())
         return name;

      long interfaceId = second ? link.getInterfaceId2() : link.getInterfaceId1();
      return (interfaceId > 0) ? session.getObjectName(interfaceId) : null;
   }

   /**
    * Get formatted values of data sources at given location. For a group: one row per member that has values there, each
    * row prefixed with the member's connector name on that end (for OBJECT1/OBJECT2) or the member's identity (for
    * CENTER).
    *
    * @param link map link
    * @param location location on the link
    * @param session client session
    * @param values DCI value provider
    * @return label text, empty string if there are no values
    */
   public static String locationValues(NetworkMapLink link, LinkDataLocation location, NXCSession session, LinkDciValueProvider values)
   {
      if (!(link instanceof ParallelLinkGroup))
         return values.getDciDataAsString(link, location);

      ParallelLinkGroup group = (ParallelLinkGroup)link;
      StringBuilder sb = new StringBuilder();
      for(NetworkMapLink member : group.getLinks())
      {
         String text = values.getDciDataAsString(member, group.memberLocation(member, location));
         if (text.isEmpty())
            continue;
         if (sb.length() > 0)
            sb.append('\n');
         String prefix = (location == LinkDataLocation.CENTER) ? memberIdentity(group, member, session)
               : connectorName(member, (location == LinkDataLocation.OBJECT2) != group.isInverted(member), session);
         if (prefix != null)
            sb.append(prefix).append(": ");
         sb.append(text.replace("\n", "  "));
      }
      return sb.toString();
   }

   /**
    * Get text of the center label: link name and CENTER values. For a group: member count, then one row per member that
    * has a name or CENTER values.
    *
    * @param link map link
    * @param session client session
    * @param values DCI value provider
    * @return label text, empty string if there is nothing to show
    */
   public static String centerLabel(NetworkMapLink link, NXCSession session, LinkDciValueProvider values)
   {
      if (link instanceof ParallelLinkGroup)
      {
         ParallelLinkGroup group = (ParallelLinkGroup)link;
         StringBuilder sb = new StringBuilder(i18n.tr("{0} links", group.getLinks().size()));
         for(NetworkMapLink member : group.getLinks())
         {
            String text = values.getDciDataAsString(member, LinkDataLocation.CENTER);
            if (!member.hasName() && text.isEmpty())
               continue;
            sb.append('\n').append(memberIdentity(group, member, session));
            if (!text.isEmpty())
               sb.append(": ").append(text.replace("\n", "  "));
         }
         return sb.toString();
      }

      StringBuilder sb = new StringBuilder();
      if (link.hasName())
         sb.append(link.getName());
      String text = values.getDciDataAsString(link, LinkDataLocation.CENTER);
      if (!text.isEmpty())
      {
         if (sb.length() > 0)
            sb.append('\n');
         sb.append(text);
      }
      return sb.toString();
   }

   /**
    * Identity of a group member for display: its name if set, otherwise connector names on both ends ordered as the
    * group's ends.
    *
    * @param group parallel link group
    * @param member member link
    * @param session client session
    * @return member identity
    */
   public static String memberIdentity(ParallelLinkGroup group, NetworkMapLink member, NXCSession session)
   {
      if (member.hasName())
         return member.getName();
      boolean inverted = group.isInverted(member);
      String name1 = connectorName(member, inverted, session);
      String name2 = connectorName(member, !inverted, session);
      return ((name1 != null) ? name1 : i18n.tr("<unknown>")) + " - " + ((name2 != null) ? name2 : i18n.tr("<unknown>"));
   }
}
