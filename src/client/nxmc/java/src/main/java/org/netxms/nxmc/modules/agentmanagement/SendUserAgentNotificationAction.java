/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.agentmanagement.dialogs.SendUserAgentNotificationDialog;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Action for sending user agent notification
 */
public class SendUserAgentNotificationAction extends ObjectAction<AbstractObject>
{
   private final I18n i18n = LocalizationHelper.getI18n(SendUserAgentNotificationAction.class);

   /**
    * Create action for sending user agent notification
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public SendUserAgentNotificationAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(AbstractObject.class, LocalizationHelper.getI18n(SendUserAgentNotificationAction.class).tr("Send user support application notification"), viewPlacement, selectionProvider);
      setImageDescriptor(ResourceManager.getImageDescriptor("icons/user_agent_messages.png"));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<AbstractObject> objects)
   {
      final NXCSession session = Registry.getSession();
      Set<Long> objectIds = new HashSet<Long>();
      for (Object obj : objects)
      {
         objectIds.add(((AbstractObject)obj).getObjectId());
      }
      
      final SendUserAgentNotificationDialog dlg = new SendUserAgentNotificationDialog(getShell());
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Create user support application notification job"), null, getMessageArea()) {
            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Failed to create user support application notification");
            }

            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.createUserAgentNotification(dlg.getMessage(), objectIds, dlg.getStartTime(), dlg.getEndTime(), dlg.isStartupNotification());
            }
         }.start();
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      if (!selection.isEmpty())
      {
         for (Object obj : ((IStructuredSelection)selection).toList())
         {
            if (!(((obj instanceof AbstractNode) && ((AbstractNode)obj).hasAgent()) || (obj instanceof Collector) ||
                (obj instanceof Container) || (obj instanceof ServiceRoot) || (obj instanceof Rack) ||
                (obj instanceof Cluster)))
            {
               return false;
            }
         }
      }
      else
      {
         return false;
      }
      return true;
   }
}
