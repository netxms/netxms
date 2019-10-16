/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.agentmanager.actions;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.dialogs.SendUserAgentNotificationDialog;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Send action for user agent notification
 */
public class SendUserAgentNotification implements IObjectActionDelegate
{
   private Shell shell;
   private ViewPart viewPart;
	private Set<Long> objects = null;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{	
	   final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      final SendUserAgentNotificationDialog dlg = new SendUserAgentNotificationDialog(shell);
      if (dlg.open() == Window.OK)
      {
         new ConsoleJob("Create user agent notification job", viewPart, Activator.PLUGIN_ID, null) {
            @Override
            protected String getErrorMessage()
            {
               return "Failed to create user agent notification";
            }

            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.createUserAgentNotification(dlg.getMessage(), objects.toArray(new Long[objects.size()]), dlg.getStartTime(), dlg.getEndTime());
            }
         }.start();
      }
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
	   boolean isEnabled = true;
	   Set<Long> tmp = new HashSet<Long>();
		if (!selection.isEmpty() && (selection instanceof IStructuredSelection))
		{
		   for (Object obj : ((IStructuredSelection)selection).toList())
		   {
   			if (((obj instanceof AbstractNode) && ((AbstractNode)obj).hasAgent()) || 
   			    (obj instanceof Container) || (obj instanceof ServiceRoot) || (obj instanceof Rack) ||
   			    (obj instanceof Cluster))
   			{
   			   tmp.add(((AbstractObject)obj).getObjectId());
   			}
   			else
   			{
   			   isEnabled = false;
   			   break;
   			}
		   }
		}
		else
		{
         isEnabled = false;
		}
      action.setEnabled(isEnabled);
      if (isEnabled)
         objects = tmp;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
   public void setActivePart(IAction action, IWorkbenchPart targetPart)
   {
      shell = targetPart.getSite().getShell();
      viewPart = (targetPart instanceof ViewPart) ? (ViewPart)targetPart : null;
   }
}
