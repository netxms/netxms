/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.objects.actions;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Action for manually setting interface peer
 */
public class SetInterfacePeerInformation extends ObjectAction<Interface>
{
   private final I18n i18n = LocalizationHelper.getI18n(SetInterfacePeerInformation.class);

   /**
    * Create action for creating interface DCIs.
    *
    * @param text action's text
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public SetInterfacePeerInformation(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(Interface.class, LocalizationHelper.getI18n(SetInterfacePeerInformation.class).tr("Set peer..."), viewPlacement, selectionProvider);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<Interface> objects)
   {
      if (objects.size() == 0)
         return;      

      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), ObjectSelectionDialog.createInterfaceSelectionFilter());
      if (dlg.open() != Window.OK)
         return;

      AbstractObject[] selection = dlg.getSelectedObjects(Interface.class);
      if (selection.length == 0)
         return;

      final Interface peerInterface = (Interface)selection[0];
      final Interface thisInterface = objects.get(0);
      if ((thisInterface.getPeerInterfaceId() != 0) || (peerInterface.getPeerInterfaceId() != 0))
      {
         if (!MessageDialogHelper.openConfirm(getShell(), i18n.tr("Warning"), i18n.tr("You are about to change existing peer information on interface. Are you sure?")))
            return; // Do not change existing information
      }

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Update peer for interfaces"), null, getMessageArea()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.setInterfacePeer(thisInterface.getObjectId(), peerInterface.getObjectId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update peer for interface");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      return (selection.size() == 1) && (selection.getFirstElement() instanceof Interface);
   }
}
