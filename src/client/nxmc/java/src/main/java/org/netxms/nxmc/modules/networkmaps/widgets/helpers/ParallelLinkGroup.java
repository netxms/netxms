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
import java.util.Collection;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.netxms.client.maps.LinkDataLocation;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.configs.MapLinkDataSource;

/**
 * Single drawn link standing in for several parallel links of type NORMAL between the same two map elements. Exists only
 * on the client side for display; it is never part of the map page and never saved.
 */
public class ParallelLinkGroup extends NetworkMapLink
{
   /**
    * Threshold used by map views that have no network map object to read it from.
    */
   public static final int DEFAULT_MERGE_THRESHOLD = 2;

   private final List<NetworkMapLink> links;

   /**
    * Create group from member links. Members must all connect the same pair of elements (in either orientation).
    *
    * @param links member links
    */
   private ParallelLinkGroup(List<NetworkMapLink> links)
   {
      super(0, NetworkMapLink.MULTILINK, links.get(0).getElement1(), links.get(0).getElement2());
      this.links = links;

      List<MapLinkDataSource> dciList = new ArrayList<MapLinkDataSource>();
      List<Long> statusObjects = new ArrayList<Long>();
      for(NetworkMapLink link : links)
      {
         boolean inverted = isInverted(link);
         for(MapLinkDataSource dci : link.getDciAsList())
         {
            if (inverted)
               dci.setLocation(invert(dci.getLocation()));
            dciList.add(dci);
         }
         for(Long id : link.getStatusObjects())
         {
            if (!statusObjects.contains(id))
               statusObjects.add(id);
         }
      }
      getConfig().setDciList(dciList.toArray(new MapLinkDataSource[dciList.size()]));
      setStatusObjects(statusObjects);
   }

   /**
    * Replace each set of more than {@code threshold} NORMAL links between the same pair of elements with a single group
    * link. Other links are passed through. Fan-out of all returned links is recomputed.
    *
    * @param links all links of the map page
    * @param threshold group size above which links are merged (0 or negative disables merging)
    * @return links to display
    */
   public static List<NetworkMapLink> merge(Collection<NetworkMapLink> links, int threshold)
   {
      List<NetworkMapLink> result = new ArrayList<NetworkMapLink>(links.size());
      if (threshold <= 0)
      {
         result.addAll(links);
         updateFanOut(result);
         return result;
      }

      Map<Long, List<NetworkMapLink>> candidates = new LinkedHashMap<Long, List<NetworkMapLink>>();
      for(NetworkMapLink link : links)
      {
         if (link.getType() != NetworkMapLink.NORMAL)
         {
            result.add(link);
            continue;
         }
         Long key = pairKey(link);
         List<NetworkMapLink> group = candidates.get(key);
         if (group == null)
         {
            group = new ArrayList<NetworkMapLink>();
            candidates.put(key, group);
         }
         group.add(link);
      }

      for(List<NetworkMapLink> group : candidates.values())
      {
         if (group.size() > threshold)
         {
            group.sort(Comparator.comparingLong(NetworkMapLink::getId));
            result.add(new ParallelLinkGroup(group));
         }
         else
         {
            result.addAll(group);
         }
      }
      updateFanOut(result);
      return result;
   }

   /**
    * Recompute position and duplicate count of given links, so that links between the same pair of elements are spread
    * evenly.
    *
    * @param links links to display
    */
   private static void updateFanOut(List<NetworkMapLink> links)
   {
      Map<Long, List<NetworkMapLink>> pairs = new LinkedHashMap<Long, List<NetworkMapLink>>();
      for(NetworkMapLink link : links)
      {
         Long key = pairKey(link);
         List<NetworkMapLink> pair = pairs.get(key);
         if (pair == null)
         {
            pair = new ArrayList<NetworkMapLink>();
            pairs.put(key, pair);
         }
         pair.add(link);
      }
      for(List<NetworkMapLink> pair : pairs.values())
      {
         long commonFirstElement = pair.get(0).getElement1();
         for(int i = 0; i < pair.size(); i++)
         {
            NetworkMapLink link = pair.get(i);
            link.resetPosition();
            if (pair.size() > 1)
            {
               link.setCommonFirstElement(commonFirstElement);
               link.setDuplicateCount(pair.size() - 1);
               link.setPosition(i);
            }
         }
      }
   }

   /**
    * @return key identifying the unordered pair of elements connected by given link
    */
   private static Long pairKey(NetworkMapLink link)
   {
      long low = Math.min(link.getElement1(), link.getElement2());
      long high = Math.max(link.getElement1(), link.getElement2());
      return (low << 32) | (high & 0xFFFFFFFFL);
   }

   /**
    * @return member links ordered by link ID
    */
   public List<NetworkMapLink> getLinks()
   {
      return links;
   }

   /**
    * Check if given member link has its elements in opposite order relative to this group.
    *
    * @param link member link
    * @return true if member's element1 is this group's element2
    */
   public boolean isInverted(NetworkMapLink link)
   {
      return link.getElement1() == getElement2();
   }

   /**
    * Get location on a member link that corresponds to given location on this group.
    *
    * @param link member link
    * @param location location on this group
    * @return location on member link
    */
   public LinkDataLocation memberLocation(NetworkMapLink link, LinkDataLocation location)
   {
      return isInverted(link) ? invert(location) : location;
   }

   /**
    * Swap OBJECT1 and OBJECT2 locations.
    *
    * @param location location to swap
    * @return swapped location
    */
   private static LinkDataLocation invert(LinkDataLocation location)
   {
      switch(location)
      {
         case OBJECT1:
            return LinkDataLocation.OBJECT2;
         case OBJECT2:
            return LinkDataLocation.OBJECT1;
         default:
            return location;
      }
   }
}
