/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSolutions
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
package org.netxms.ui.eclipse.objectmanager.actions;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;


/**
 * Apply agent policy from node menu
 *
 */
public class ApplyPolicy implements IObjectActionDelegate
{
   private Shell shell;
   private List<AbstractObject> selectedObjects = null;

   @Override
   public void run(IAction action)
   {
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(shell, null, ObjectSelectionDialog.createPolicySelectionFilter());
      
      if (dlg.open() == Window.OK)
      {
         final NXCSession session = (NXCSession)ConsoleSharedData.getSession();

         new ConsoleJob("Deploy agent policy", null, Activator.PLUGIN_ID, null) {

            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               for(int i = 0; i < selectedObjects.size(); i++)
               {
                  for(int n = 0; n < dlg.getSelectedObjects().size(); n++)
                  {
                     session.deployAgentPolicy(dlg.getSelectedObjects().get(n).getObjectId(), selectedObjects.get(i).getObjectId());
                  }
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot deploy agent policy";
            }
         }.start();

      }
   }

   @Override
   public void selectionChanged(IAction action, ISelection selection)
   {
      if ((selection instanceof IStructuredSelection) &&
          (((IStructuredSelection)selection).size() != 0))
      {
         selectedObjects = new ArrayList<AbstractObject>();
         for (Object s : ((IStructuredSelection)selection).toList())
         {
            if ((s instanceof Node) || (s instanceof Interface) || (s instanceof AccessPoint))
            {
               action.setEnabled(true);
               selectedObjects.add(((AbstractObject)s));
            }
         }
      }
      else
      {
         action.setEnabled(false);
         selectedObjects = null;
      }
   }

   @Override
   public void setActivePart(IAction action, IWorkbenchPart targetPart)
   {
      shell = targetPart.getSite().getShell();
   }

}
