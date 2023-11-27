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
package org.netxms.nxmc.modules.objects.actions;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.SetInterfaceExpStateDlg;
import org.xnap.commons.i18n.I18n;

/**
 * Action for creating interface DCIs
 */
public class ChangeInterfaceExpectedStateAction extends ObjectAction<Interface>
{
   private final I18n i18n = LocalizationHelper.getI18n(ChangeInterfaceExpectedStateAction.class);

   /**
    * Create action for creating interface DCIs.
    *
    * @param text action's text
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public ChangeInterfaceExpectedStateAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(Interface.class, LocalizationHelper.getI18n(ChangeInterfaceExpectedStateAction.class).tr("Change interface e&xpected state..."), viewPlacement, selectionProvider);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<Interface> objects)
   {

      if (objects.size() == 0)
         return;
      
      SetInterfaceExpStateDlg dlg = new SetInterfaceExpStateDlg(getShell());
      if (dlg.open() != Window.OK)
         return;
      
      final long[] idList = new long[objects.size()];
      for(int i = 0; i < idList.length; i++)
         idList[i] = objects.get(i).getObjectId();
      final int newState = dlg.getExpectedState();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Update expected state for interfaces"), null, getMessageArea()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < idList.length; i++)
            {
               NXCObjectModificationData md = new NXCObjectModificationData(idList[i]);
               md.setExpectedState(newState);
               session.modifyObject(md);
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update expected state for interface object");
         }
      }.start();
   }
}
