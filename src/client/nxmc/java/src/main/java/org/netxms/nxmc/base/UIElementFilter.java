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
package org.netxms.nxmc.base;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.netxms.base.Glob;
import org.netxms.client.NXCSession;

/**
 * UI element filer (based on UI access rules received from server)
 */
public class UIElementFilter
{
   public static enum ElementType
   {
      ANY, PERSPECTIVE, VIEW
   }

   private boolean includeAll = false;
   private Map<ElementType, Filter> filters = new HashMap<>();

   /**
    * Create element UI filter for given session.
    * 
    * @param session client session
    */
   public UIElementFilter(NXCSession session)
   {
      String rules = session.getUIAccessRules();
      if ((rules == null) || rules.isBlank())
         return;

      for(String r : rules.toLowerCase().split(";"))
      {
         r = r.trim();
         if (r.isEmpty())
            continue;

         if (r.equals("*") || r.equals("*:*"))
            includeAll = true;

         int set;
         if (r.charAt(0) == '!')
         {
            set = 2;
            r = r.substring(1);
         }
         else if (r.charAt(0) == '^')
         {
            set = 1;
            r = r.substring(1);
         }
         else
         {
            if (includeAll)
               continue; // Already have * in includes, no point in processing individual includes
            set = 0;
         }

         String[] parts = r.split(":", 2);
         if ((parts.length != 2) || parts[0].isBlank() || parts[1].isBlank())
            continue;

         ElementType type;
         switch(parts[0].toLowerCase())
         {
            case "p":
            case "perspective":
               type = ElementType.PERSPECTIVE;
               break;
            case "v":
            case "view":
               type = ElementType.VIEW;
               break;
            case "*":
               type = ElementType.ANY;
               break;
            default:
               type = null;
               break;
         }

         if (type == null)
            continue;

         Filter f = filters.get(type);
         if (f == null)
         {
            f = new Filter();
            filters.put(type, f);
         }

         boolean isPattern = parts[1].contains("*") || parts[1].contains("?");
         if (set == 2)
         {
            if (isPattern)
               f.exclusionPatterns.add(parts[1]);
            else
               f.exclusions.add(parts[1]);
         }
         else if (set == 1)
         {
            if (isPattern)
               f.priorityInclusionPatterns.add(parts[1]);
            else
               f.priorityInclusions.add(parts[1]);
         }
         else
         {
            if (isPattern)
               f.inclusionPatterns.add(parts[1]);
            else
               f.inclusions.add(parts[1]);
         }
      }
   }

   /**
    * Check if element with given ID is visible
    *
    * @param type element type
    * @param id element ID
    * @return true if element is visible
    */
   public boolean isVisible(ElementType type, String id)
   {
      id = id.trim().toLowerCase();

      Filter ft = filters.get(type);
      if (ft != null)
      {
         if (ft.matchPriorityInclusions(id))
            return true;
         if (ft.matchExclusions(id))
            return false;
      }

      Filter fa = filters.get(ElementType.ANY);
      if (fa != null)
      {
         if (fa.matchPriorityInclusions(id))
            return true;
         if (fa.matchExclusions(id))
            return false;
      }

      if (includeAll)
         return true;

      if ((ft != null) && ft.matchInclusions(id))
         return true;

      if ((fa != null) && fa.matchInclusions(id))
         return true;

      return false;
   }

   /**
    * Filter for specific element type
    */
   private static class Filter
   {
      Set<String> exclusions = new HashSet<>();
      Set<String> inclusions = new HashSet<>();
      Set<String> priorityInclusions = new HashSet<>();
      List<String> exclusionPatterns = new ArrayList<>(0);
      List<String> inclusionPatterns = new ArrayList<>(0);
      List<String> priorityInclusionPatterns = new ArrayList<>(0);

      private static boolean match(String id, Set<String> identifiers, List<String> patterns)
      {
         if (identifiers.contains(id))
            return true;
         for(String p : patterns)
            if (Glob.match(p, id))
               return true;
         return false;
      }

      /**
       * Check if given ID is an exclusion (explicitly forbidden)
       *
       * @param id element ID
       * @return true if given ID is an exclusion
       */
      public boolean matchExclusions(String id)
      {
         return match(id, exclusions, exclusionPatterns);
      }

      /**
       * Check if given ID is an inclusion (explicitly allowed)
       *
       * @param id element ID
       * @return true if given ID is an inclusion
       */
      public boolean matchInclusions(String id)
      {
         return match(id, inclusions, inclusionPatterns);
      }

      /**
       * Check if given ID is a priority inclusion (explicitly allowed)
       *
       * @param id element ID
       * @return true if given ID is an inclusion
       */
      boolean matchPriorityInclusions(String id)
      {
         return match(id, priorityInclusions, priorityInclusionPatterns);
      }
   }
}
