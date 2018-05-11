/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.policymanager;

import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.client.objects.AgentPolicyConfig;
import org.netxms.client.objects.AgentPolicyLogParser;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler;
import org.netxms.ui.eclipse.policymanager.views.AbstractPolicyEditor;
import org.netxms.ui.eclipse.policymanager.views.ConfigPolicyEditor;
import org.netxms.ui.eclipse.policymanager.views.LogParserPolicyEditor;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Open handler for policy objects
 */
public class PolicyOpenHandler implements ObjectOpenHandler
{
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectbrowser.api.ObjectOpenHandler#openObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean openObject(AbstractObject object)
   {
      if (!(object instanceof AgentPolicy))
         return false;
      
      final IWorkbenchWindow window = PlatformUI.getWorkbench().getActiveWorkbenchWindow();
      try
      {
         AbstractPolicyEditor view = null;
         if (object instanceof AgentPolicyConfig)
            view = (AbstractPolicyEditor)window.getActivePage().showView(ConfigPolicyEditor.ID, Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
         else if (object instanceof AgentPolicyLogParser)
            view = (AbstractPolicyEditor)window.getActivePage().showView(LogParserPolicyEditor.ID, Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
         if (view != null)
            view.setPolicy((AgentPolicy)object);
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(window.getShell(), "Error", String.format("Cannot open policy editor: %s", e.getLocalizedMessage()));
      }                       
      return true;
   }
}
