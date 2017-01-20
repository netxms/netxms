/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.eclipse.topology.actions;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.dialogs.EnterPrimaryHostnameDlg;

/**
 * Find primary hostname in the network
 * 
 */
public class FindNodeByPrimaryHostname implements IWorkbenchWindowActionDelegate
{
   private IWorkbenchWindow window;

   /* (non-Javadoc)
    * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
    */
   @Override
   public void run(IAction action)
   {
      final EnterPrimaryHostnameDlg dlg = new EnterPrimaryHostnameDlg(window.getShell());
      if (dlg.open() != Window.OK)
         return;

      final Shell shell = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell();      
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob("Searching for hostname" + dlg.getHostname() + "in the network", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<AbstractNode> nodes = session.findNodesByHostname((int)dlg.getZoneId(), dlg.getHostname());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (nodes != null && !nodes.isEmpty())
                  {
                     StringBuilder sb = new StringBuilder();
                     sb.append("The following nodes matching the criteria have been found: ");
                     for(int i = 0; i < nodes.size(); i++)
                     {
                        sb.append("\n" + nodes.get(i).getPrimaryName());
                     }
                     MessageDialogHelper.openInformation(shell, "Result", sb.toString());
                  }
                  else
                     MessageDialogHelper.openInformation(shell, "Result", "No nodes found!");
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Search for hostname" + dlg.getHostname() + "failed.";
         }
      }.start();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void selectionChanged(IAction action, ISelection selection)
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#dispose()
    */
   @Override
   public void dispose()
   {  
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#init(org.eclipse.ui.IWorkbenchWindow)
    */
   @Override
   public void init(IWorkbenchWindow window)
   {
      this.window = window;
   }
   
}