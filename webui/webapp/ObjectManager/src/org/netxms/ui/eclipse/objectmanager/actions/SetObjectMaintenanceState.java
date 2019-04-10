/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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

import org.eclipse.core.commands.AbstractHandler;
import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.handlers.HandlerUtil;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.imagelibrary.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objects.ObjectWrapper;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Set object management state
 */
class SetObjectMaintenanceState extends AbstractHandler
{
   private boolean enter;
   
   /**
    * @param manage
    */
   protected SetObjectMaintenanceState(boolean enter)
   {
      super();
      this.enter = enter;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.core.commands.AbstractHandler#execute(org.eclipse.core.commands.ExecutionEvent)
    */
   @Override
   public Object execute(ExecutionEvent event) throws ExecutionException
   {
      IWorkbenchWindow window = HandlerUtil.getActiveWorkbenchWindow(event);
      ISelection selection = window.getActivePage().getSelection();
      if ((selection == null) || !(selection instanceof IStructuredSelection) || selection.isEmpty())
         return null;
      
      final String comments; 
      if (enter)
      {
         InputDialog dlg = new InputDialog(null, "Enter Maintenance", "Additional comments", "", null);
         if (dlg.open() != Window.OK)
            return null;
         comments = dlg.getValue().trim();
      }
      else
      {
         comments = null;
      }
      
      final Object[] objects = ((IStructuredSelection)selection).toArray();
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob(Messages.get().SetObjectManagementState_JobTitle, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               if (o instanceof AbstractObject)
                  session.setObjectMaintenanceMode(((AbstractObject)o).getObjectId(), enter, comments);
               else if (o instanceof ObjectWrapper)
                  session.setObjectMaintenanceMode(((ObjectWrapper)o).getObjectId(), enter, comments);
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().SetObjectManagementState_JobError;
         }
      }.start();
      return null;
   }
}
