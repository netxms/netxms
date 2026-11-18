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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;

/**
 * Group of data collection elements (DCI values or data collection objects) shown as a tree node in data collection views.
 * Groups are equal if they have the same key, so expansion state survives rebuilding groups on refresh. Fallback group holds
 * elements without value for grouping attribute (for example DCIs without tag) and is sorted after all other groups.
 */
public class DciGroup
{
   private static final Object FALLBACK_KEY = new Object();

   private final Object key;
   private final String name;
   private final boolean fallback;
   private final long objectId;
   private final List<Object> elements = new ArrayList<>();

   /**
    * Create new group.
    *
    * @param key group key (unique within grouping mode)
    * @param name group display name
    */
   public DciGroup(Object key, String name)
   {
      this.key = key;
      this.name = name;
      this.fallback = false;
      this.objectId = 0;
   }

   /**
    * Create new group for elements referencing given object (for example template or related object).
    *
    * @param objectId ID of referenced object (used as group key)
    * @param name group display name
    */
   public DciGroup(long objectId, String name)
   {
      this.key = Long.valueOf(objectId);
      this.name = name;
      this.fallback = false;
      this.objectId = objectId;
   }

   /**
    * Create fallback group for elements without value for grouping attribute.
    *
    * @param name group display name
    */
   public DciGroup(String name)
   {
      this.key = FALLBACK_KEY;
      this.name = name;
      this.fallback = true;
      this.objectId = 0;
   }

   /**
    * Add element to group.
    *
    * @param element element to add
    */
   void add(Object element)
   {
      elements.add(element);
   }

   /**
    * Get group display name.
    *
    * @return group display name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get ID of object referenced by group elements.
    *
    * @return ID of referenced object or 0 if group does not represent an object
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * Get group elements.
    *
    * @return group elements
    */
   public Object[] getElements()
   {
      return elements.toArray();
   }

   /**
    * Check if any element of this group passes given filter. Viewer filters use this to hide groups without visible elements.
    *
    * @param filter filter to check
    * @param viewer viewer
    * @return true if at least one element passes filter
    */
   public boolean hasVisibleElements(ViewerFilter filter, Viewer viewer)
   {
      for(Object e : elements)
      {
         if (filter.select(viewer, this, e))
            return true;
      }
      return false;
   }

   /**
    * Compare two groups. Groups are sorted by name, with fallback group placed last.
    *
    * @param g1 first group
    * @param g2 second group
    * @return comparison result
    */
   public static int compare(DciGroup g1, DciGroup g2)
   {
      if (g1.fallback != g2.fallback)
         return g1.fallback ? 1 : -1;
      return g1.name.compareToIgnoreCase(g2.name);
   }

   /**
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      return key.hashCode();
   }

   /**
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj)
   {
      if (this == obj)
         return true;
      if (!(obj instanceof DciGroup))
         return false;
      return key.equals(((DciGroup)obj).key);
   }
}
