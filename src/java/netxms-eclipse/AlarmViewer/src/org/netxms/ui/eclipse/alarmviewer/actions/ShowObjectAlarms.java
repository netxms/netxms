/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer.actions;

import org.eclipse.core.commands.AbstractHandler;
import org.eclipse.core.commands.ExecutionEvent;
import org.eclipse.core.commands.ExecutionException;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.handlers.HandlerUtil;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.alarmviewer.views.ObjectAlarmBrowser;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Handler for "show object alarms" command
 */
public class ShowObjectAlarms extends AbstractHandler
{
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
      
      StringBuilder sb = new StringBuilder();
      for(Object o : ((IStructuredSelection)selection).toList())
      {
         if (!(o instanceof AbstractObject))
            continue;
         if (sb.length() > 0)
            sb.append('&');
         sb.append(((AbstractObject)o).getObjectId());
      }
      try
      {
         window.getActivePage().showView(ObjectAlarmBrowser.ID, sb.toString(), IWorkbenchPage.VIEW_ACTIVATE);
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(window.getShell(), Messages.get().ShowObjectAlarms_Error, Messages.get().ShowObjectAlarms_ErrorOpeningView + e.getMessage());
      }
      
      return null;
   }
}
