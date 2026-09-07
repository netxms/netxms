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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for event processing policy rule lists (columns: chain, rule number, rule name)
 */
public class RuleLabelProvider extends LabelProvider implements ITableLabelProvider
{
   public static final int COLUMN_CHAIN = 0;
   public static final int COLUMN_NUMBER = 1;
   public static final int COLUMN_NAME = 2;

   private final I18n i18n = LocalizationHelper.getI18n(RuleLabelProvider.class);
   private EventProcessingPolicy policy;

   /**
    * @param policy policy the listed rules belong to (source of chain names); can be set later with setPolicy
    */
   public RuleLabelProvider(EventProcessingPolicy policy)
   {
      this.policy = policy;
   }

   /**
    * Set policy used to resolve chain names
    *
    * @param policy policy the listed rules belong to
    */
   public void setPolicy(EventProcessingPolicy policy)
   {
      this.policy = policy;
   }

   /**
    * Get name of chain owning given rule
    *
    * @param rule policy rule
    * @return chain name
    */
   public String getChainName(EventProcessingPolicyRule rule)
   {
      if (rule.getChainId() == 0)
         return i18n.tr("Main");
      EventProcessingPolicyChain chain = (policy != null) ? policy.findChain(rule.getChainId()) : null;
      return (chain != null) ? chain.getName() : Integer.toString(rule.getChainId());
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
      EventProcessingPolicyRule rule = (EventProcessingPolicyRule)element;
      switch(columnIndex)
      {
         case COLUMN_CHAIN:
            return getChainName(rule);
         case COLUMN_NUMBER:
            return Integer.toString(rule.getRuleNumber());
         case COLUMN_NAME:
            return rule.getComments();
      }
      return null;
	}
}
