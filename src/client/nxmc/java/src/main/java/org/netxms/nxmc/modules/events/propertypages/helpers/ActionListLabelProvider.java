/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.propertypages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ServerAction;
import org.netxms.client.events.ActionExecutionConfiguration;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.actions.views.helpers.DecoratingActionLabelProvider;
import org.netxms.nxmc.modules.events.views.EventProcessingPolicyEditor;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for action list on action configuration page
 */
public class ActionListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private final I18n i18n = LocalizationHelper.getI18n(ActionListLabelProvider.class);
   EventProcessingPolicyEditor policyEditor;
   DecoratingActionLabelProvider actionLabelProvider;

   /**
    * @param policyEditor
    */
   public ActionListLabelProvider(EventProcessingPolicyEditor policyEditor)
   {
      this.policyEditor = policyEditor;
      this.actionLabelProvider = new DecoratingActionLabelProvider();
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex != 0)
         return null;
      ActionExecutionConfiguration c = (ActionExecutionConfiguration)element;
      ServerAction action = policyEditor.findActionById(c.getActionId()); 
      return (action != null) ? actionLabelProvider.getImage(action) : null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      ActionExecutionConfiguration c = (ActionExecutionConfiguration)element;
      switch(columnIndex)
      {
         case 0:
            ServerAction action = policyEditor.findActionById(c.getActionId());
            return (action != null) ? actionLabelProvider.getText(action) : null;
         case 1:
            return c.isActive() ? i18n.tr("Active") : i18n.tr("Inactive");
         case 2:
            return c.getTimerDelay();
         case 3:
            return c.getTimerKey();
         case 4:
            return c.getSnoozeTime();
         case 5:
            return c.getBlockingTimerKey();
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      actionLabelProvider.dispose();
      super.dispose();
   }
}
