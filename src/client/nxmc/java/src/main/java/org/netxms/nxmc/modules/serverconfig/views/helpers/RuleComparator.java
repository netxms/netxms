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
package org.netxms.nxmc.modules.serverconfig.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.netxms.client.events.EventProcessingPolicyRule;

/**
 * Comparator for event processing policy rule lists: main chain first, other chains by name, rules by number within a chain
 */
public class RuleComparator extends ViewerComparator
{
   private final RuleLabelProvider labelProvider;

   /**
    * @param labelProvider label provider resolving chain names
    */
   public RuleComparator(RuleLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
      EventProcessingPolicyRule r1 = (EventProcessingPolicyRule)e1;
      EventProcessingPolicyRule r2 = (EventProcessingPolicyRule)e2;
      if (r1.getChainId() != r2.getChainId())
      {
         if (r1.getChainId() == 0)
            return -1;
         if (r2.getChainId() == 0)
            return 1;
         int result = labelProvider.getChainName(r1).compareToIgnoreCase(labelProvider.getChainName(r2));
         if (result != 0)
            return result;
      }
      return r1.getRuleNumber() - r2.getRuleNumber();
	}
}
