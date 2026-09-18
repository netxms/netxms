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

import java.util.ArrayList;
import java.util.List;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.LinkDataDirection;
import org.netxms.client.maps.LinkDataLocation;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Text of network map link labels, shared by the graph canvas and the geographical canvas. Each label location (connector
 * name at either end, OBJECT1/OBJECT2 values, center) is built separately; for a {@link ParallelLinkGroup} every location
 * lists its members, one line per member.
 */
public final class LinkLabelText
{
   private static final LinkDataDirection[] DIRECTION_ORDER = { LinkDataDirection.FORWARD, LinkDataDirection.REVERSE, LinkDataDirection.NONE };

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
    * Get formatted values of data sources at given location. For a group: one line per member with values there,
    * followed by the member's connector name (OBJECT1/OBJECT2) or identity (CENTER) in parentheses.
    *
    * @param link map link
    * @param location location on the link
    * @param session client session
    * @param values DCI value provider
    * @return label lines, empty list if there are no values
    */
   public static List<LinkLabelLine> locationValues(NetworkMapLink link, LinkDataLocation location, NXCSession session, LinkDciValueProvider values)
   {
      if (!(link instanceof ParallelLinkGroup))
         return values.getDciData(link, location);

      ParallelLinkGroup group = (ParallelLinkGroup)link;
      List<LinkLabelLine> lines = new ArrayList<>();
      for(NetworkMapLink member : group.getLinks())
      {
         List<LinkLabelLine> memberValues = values.getDciData(member, group.memberLocation(member, location));
         if (memberValues.isEmpty())
            continue;
         String name = (location == LinkDataLocation.CENTER) ? memberIdentity(group, member, session)
               : connectorName(member, (location == LinkDataLocation.OBJECT2) != group.isInverted(member), session);
         lines.add(memberLine(name, memberValues, group.isInverted(member)));
      }
      return lines;
   }

   /**
    * Get lines of the center label: link name and CENTER values. For a group: member count, then one line per member
    * that has a name or CENTER values.
    *
    * @param link map link
    * @param session client session
    * @param values DCI value provider
    * @return label lines, empty list if there is nothing to show
    */
   public static List<LinkLabelLine> centerLabel(NetworkMapLink link, NXCSession session, LinkDciValueProvider values)
   {
      List<LinkLabelLine> lines = new ArrayList<>();
      if (link instanceof ParallelLinkGroup)
      {
         ParallelLinkGroup group = (ParallelLinkGroup)link;
         lines.add(new LinkLabelLine(LocalizationHelper.getI18n(LinkLabelText.class).tr("{0} links", group.getLinks().size())));
         for(NetworkMapLink member : group.getLinks())
         {
            List<LinkLabelLine> memberValues = values.getDciData(member, LinkDataLocation.CENTER);
            if (member.hasName() || !memberValues.isEmpty())
               lines.add(memberLine(memberIdentity(group, member, session), memberValues, group.isInverted(member)));
         }
         return lines;
      }

      if (link.hasName())
         lines.add(new LinkLabelLine(link.getName()));
      lines.addAll(values.getDciData(link, LinkDataLocation.CENTER));
      return lines;
   }

   /**
    * Build single line for a group member: its values followed by the member name in parentheses, so values of all
    * members start at the same position. Data direction of an inverted member is reversed to match the group's
    * orientation, and values are ordered by direction (forward, reverse, no direction) so arrows of all members line up.
    *
    * @param name member name (can be null)
    * @param memberValues member's values
    * @param inverted true if member's element order is opposite to the group's
    * @return line for the member
    */
   private static LinkLabelLine memberLine(String name, List<LinkLabelLine> memberValues, boolean inverted)
   {
      LinkLabelLine line = new LinkLabelLine();
      for(LinkDataDirection direction : DIRECTION_ORDER)
      {
         for(LinkLabelLine value : memberValues)
         {
            for(LinkLabelLine.Segment s : value.getSegments())
            {
               LinkDataDirection groupDirection = s.direction;
               if (inverted && (groupDirection != LinkDataDirection.NONE))
                  groupDirection = (groupDirection == LinkDataDirection.FORWARD) ? LinkDataDirection.REVERSE : LinkDataDirection.FORWARD;
               if (groupDirection == direction)
                  line.add(groupDirection, s.text);
            }
         }
      }
      if (name != null)
         line.add(LinkDataDirection.NONE, memberValues.isEmpty() ? name : "(" + name + ")");
      return line;
   }

   /**
    * Display name of a group member: link name if set, otherwise connector names of both ends in the group's
    * orientation.
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
      I18n i18n = LocalizationHelper.getI18n(LinkLabelText.class);
      return ((name1 != null) ? name1 : i18n.tr("<unknown>")) + " - " + ((name2 != null) ? name2 : i18n.tr("<unknown>"));
   }
}
