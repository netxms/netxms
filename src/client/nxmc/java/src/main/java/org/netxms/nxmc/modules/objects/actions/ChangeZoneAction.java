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
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ZoneSelectionDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Action for changing objet's zone
 */
public class ChangeZoneAction extends ObjectAction<AbstractObject>
{
   private final I18n i18n = LocalizationHelper.getI18n(ChangeZoneAction.class);

   /**
    * Create action for creating changing objects zone
    *
    * @param text action's text
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public ChangeZoneAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(AbstractObject.class, LocalizationHelper.getI18n(ChangeZoneAction.class).tr("Change zone..."), viewPlacement, selectionProvider);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<AbstractObject> objects)
   {
      final ZoneSelectionDialog dlg = new ZoneSelectionDialog(getShell());
      if (dlg.open() != Window.OK)
         return;
      
      AbstractObject object = objects.get(0);
      
      final NXCSession session = Registry.getSession();
      new Job(String.format(i18n.tr("Change zone for node %s [%d]"), object.getObjectName(), object.getObjectId()), null, getMessageArea()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.changeObjectZone(object.getObjectId(), dlg.getZoneUIN());
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot change zone for node %s [%d]"), object.getObjectName(), object.getObjectId());
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      return (selection.size() == 1) && ((selection.getFirstElement() instanceof Node) || (selection.getFirstElement() instanceof Cluster));
   }
}
