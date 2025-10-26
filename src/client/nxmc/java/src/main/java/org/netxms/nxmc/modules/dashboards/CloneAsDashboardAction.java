/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DashboardBase;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.modules.objects.dialogs.CreateObjectDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Action for linking asset to object
 */
public class CloneAsDashboardAction extends ObjectAction<DashboardBase>
{
   private final I18n i18n = LocalizationHelper.getI18n(CloneAsDashboardAction.class);

   /**
    * Create action for linking asset to object.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public CloneAsDashboardAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(DashboardBase.class, LocalizationHelper.getI18n(CloneAsDashboardAction.class).tr("Clone as &dashboard..."), viewPlacement, selectionProvider);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(List<DashboardBase> objects)
   {
      final DashboardBase sourceObject = objects.get(0);

      if ((sourceObject instanceof Dashboard) && ((Dashboard)sourceObject).isTemplateInstance())
         if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Warning"), 
               i18n.tr("This instance of a template dashboard will be cloned as a regular Dashboard. Continue?")))
            return;

      final long parentId = sourceObject.getParentIdList()[0];

      final CreateObjectDialog dlg = new CreateObjectDialog(getShell(), i18n.tr("Dashboard"));
      if (dlg.open() == Window.OK)
      {
         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Cloning dashboard"), null, getMessageArea()) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_DASHBOARD, dlg.getObjectName(), parentId);
               cd.setObjectAlias(dlg.getObjectAlias());
               final long newDashboardId = session.createObject(cd);

               final NXCObjectModificationData md = new NXCObjectModificationData(newDashboardId);
               md.setDashboardElements(sourceObject.getElements());
               md.setColumnCount(sourceObject.getNumColumns());
               md.setObjectFlags(sourceObject.getFlags());
               md.setAutoBindFlags(sourceObject.getAutoBindFlags());
               md.setAutoBindFilter(sourceObject.getAutoBindFilter());

               session.modifyObject(md);
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot clone dashboard {0}", dlg.getObjectName());
            }
         }.start();
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      return (selection.size() == 1) && (selection.getFirstElement() instanceof DashboardBase);
   }
}
