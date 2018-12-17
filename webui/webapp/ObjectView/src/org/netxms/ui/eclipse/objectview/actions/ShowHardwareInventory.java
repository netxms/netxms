/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.ui.eclipse.objectview.actions;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objectview.views.HardwareInventoryView;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Show hardware inventory for a node
 */
public class ShowHardwareInventory implements IObjectActionDelegate
{
   private IWorkbenchWindow window;
   private AbstractObject object;

   /* (non-Javadoc)
    * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
    */
   @Override
   public void setActivePart(IAction action, IWorkbenchPart targetPart)
   {
      window = targetPart.getSite().getWorkbenchWindow();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
    */
   @Override
   public void run(IAction action)
   {
      if (object != null)
      {
         try
         {
            window.getActivePage().showView(HardwareInventoryView.ID, Long.toString(object.getObjectId()), IWorkbenchPage.VIEW_ACTIVATE);
         }
         catch(PartInitException e)
         {
            MessageDialogHelper.openError(window.getShell(), "Error", String.format("Error opening hardware inventory view: %s", e.getMessage()));
         }
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void selectionChanged(IAction action, ISelection selection)
   {
      if ((selection instanceof IStructuredSelection) &&
          (((IStructuredSelection)selection).size() == 1))
      {
          Object o = ((IStructuredSelection)selection).getFirstElement();
          if (o instanceof AbstractNode)
          {
             object = (AbstractObject)o;
          }
          else
          {
             object = null;
          }
      }
      else
      {
         object = null;
      }
      action.setEnabled((object != null) && (!(object instanceof AbstractNode) || ((AbstractNode)object).hasAgent()));
   }
}
