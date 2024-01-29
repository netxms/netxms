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
package org.netxms.nxmc.modules.events.views.helpers;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import org.netxms.client.events.EventProcessingPolicyRule;

/**
 * Rule clipboard
 */
public class RuleClipboard
{
   private List<EventProcessingPolicyRule> content = new ArrayList<EventProcessingPolicyRule>(0);

   /**
    * @return
    */
   public boolean isEmpty()
   {
      return content.isEmpty();
   }

   /**
    * Clear clipboard
    */
   public void clear()
   {
      content.clear();
   }

   /**
    * Add rule to clipboard. Added rule should not be modified by caller after adding to clipboard.
    * 
    * @param rule
    */
   public void add(EventProcessingPolicyRule rule)
   {
      content.add(rule);
   }

   /**
    * Do paste - returns content (cloned as necessary) ready for pasting
    * 
    * @return
    */
   public Collection<EventProcessingPolicyRule> paste()
   {
      List<EventProcessingPolicyRule> list = new ArrayList<EventProcessingPolicyRule>(content.size());
      for(EventProcessingPolicyRule r : content)
         list.add(new EventProcessingPolicyRule(r));
      return list;
   }
}
