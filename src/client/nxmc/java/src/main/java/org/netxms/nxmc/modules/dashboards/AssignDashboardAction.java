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
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.actions.ObjectAction;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Action for assigning dashboard to an object
 */
public class AssignDashboardAction extends ObjectAction<Dashboard>
{
   private final I18n i18n = LocalizationHelper.getI18n(AssignDashboardAction.class);

   /**
    * Create action for assigning dashboard to an object.
    *
    * @param viewPlacement view placement information
    * @param selectionProvider associated selection provider
    */
   public AssignDashboardAction(ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(Dashboard.class, LocalizationHelper.getI18n(AssignDashboardAction.class).tr("&Assign to object..."), viewPlacement, selectionProvider);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#run(java.util.List)
    */
   @Override
   protected void run(final List<Dashboard> dashboards)
   {
      Set<Integer> classFilter = ObjectSelectionDialog.createDataCollectionTargetSelectionFilter();
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), classFilter);
      if (dlg.open() != Window.OK)
         return;

      final List<AbstractObject> targetObjects = dlg.getSelectedObjects();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Assigning dashboard"), null, getMessageArea()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(AbstractObject object : targetObjects)
            {
               boolean modified = false;
               Set<Long> currentDashboards = object.getDashboardIdentifiers();
               for(Dashboard dashboard : dashboards)
               {
                  if (!currentDashboards.contains(dashboard.getObjectId()))
                  {
                     currentDashboards.add(dashboard.getObjectId());
                     modified = true;
                  }
               }
               if (modified)
               {
                  final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
                  md.setDashboards(currentDashboards.toArray(Long[]::new));
                  session.modifyObject(md);
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot assign dashboard");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.actions.ObjectAction#isValidForSelection(org.eclipse.jface.viewers.IStructuredSelection)
    */
   @Override
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      for(Object object : selection.toList())
      {
         if (!(object instanceof Dashboard))
            return false;
      }
      return true;
   }
}
