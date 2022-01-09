/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.helpers.MenuContributionItem;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.MaintanenceScheduleDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Helper class for building object context menu
 */
public class ObjectContextMenuManager extends MenuManager
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectContextMenuManager.class);

   private View view;
   private ISelectionProvider selectionProvider;
   private Action actionManage;
   private Action actionUnmanage;
   private Action actionEnterMaintenance;
   private Action actionLeaveMaintenance;
   private Action actionScheduleMaintenance;
   private Action actionProperties;

   /**
    * Create new obejct context menu manager.
    *
    * @param view owning view
    * @param selectionProvider selection provider
    */
   public ObjectContextMenuManager(View view, ISelectionProvider selectionProvider)
   {
      this.view = view;
      this.selectionProvider = selectionProvider;
      setRemoveAllWhenShown(true);
      addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu();
         }
      });
      createActions();
   }

   /**
    * Create object actions
    */
   private void createActions()
   {
      actionManage = new Action(i18n.tr("&Manage")) {
         @Override
         public void run()
         {
            changeObjectManagementState(true);
         }
      };

      actionUnmanage = new Action(i18n.tr("&Unmanage")) {
         @Override
         public void run()
         {
            changeObjectManagementState(false);
         }
      };

      actionEnterMaintenance = new Action(i18n.tr("&Enter maintenance mode...")) {
         @Override
         public void run()
         {
            changeObjectMaintenanceState(true);
         }
      };

      actionLeaveMaintenance = new Action(i18n.tr("&Leave maintenance mode")) {
         @Override
         public void run()
         {
            changeObjectMaintenanceState(false);
         }
      };

      actionScheduleMaintenance = new Action(i18n.tr("&Schedule maintenance...")) {
         @Override
         public void run()
         {
            scheduleMaintenance();
         }
      };

      actionProperties = new Action(i18n.tr("&Properties...")) {
         @Override
         public void run()
         {
            ObjectPropertiesManager.openObjectPropertiesDialog(getObjectFromSelection(), getShell());
         }
      };
   }

   /**
    * Fill object context menu
    */
   protected void fillContextMenu()
   {
      boolean singleObject = ((IStructuredSelection)selectionProvider.getSelection()).size() == 1;

      if (singleObject)
      {
         MenuManager createMenu = new ObjectCreateMenuManager(getShell(), view, getObjectFromSelection());
         if (!createMenu.isEmpty())
         {
            add(createMenu);
            add(new Separator());
         }
      }

      add(actionManage);
      add(actionUnmanage);

      MenuManager maintenanceMenu = new MenuManager(i18n.tr("&Maintenance"));
      maintenanceMenu.add(actionEnterMaintenance);
      maintenanceMenu.add(actionLeaveMaintenance);
      maintenanceMenu.add(actionScheduleMaintenance);
      add(maintenanceMenu);

      final Menu toolsMenu = ObjectMenuFactory.createToolsMenu((IStructuredSelection)selectionProvider.getSelection(), getMenu(), null, new ViewPlacement(view));
      if (toolsMenu != null)
      {
         add(new Separator());
         add(new MenuContributionItem(i18n.tr("&Tools"), toolsMenu));
      }

      final Menu pollsMenu = ObjectMenuFactory.createPollMenu((IStructuredSelection)selectionProvider.getSelection(), getMenu(), null, new ViewPlacement(view));
      if (pollsMenu != null)
      {
         add(new Separator());
         add(new MenuContributionItem(i18n.tr("&Poll"), pollsMenu));
      }

      if (singleObject)
      {
         add(new Separator());
         add(actionProperties);
      }
   }

   /**
    * Get parent shell for dialog windows.
    *
    * @return parent shell for dialog windows
    */
   protected Shell getShell()
   {
      return Registry.getMainWindow().getShell();
   }

   /**
    * Get object from current selection
    *
    * @return object or null
    */
   protected AbstractObject getObjectFromSelection()
   {
      IStructuredSelection selection = (IStructuredSelection)selectionProvider.getSelection();
      if (selection.size() != 1)
         return null;
      return (AbstractObject)selection.getFirstElement();
   }

   /**
    * Get object ID from selection
    *
    * @return object ID or 0
    */
   protected long getObjectIdFromSelection()
   {
      IStructuredSelection selection = (IStructuredSelection)selectionProvider.getSelection();
      if (selection.size() != 1)
         return 0;
      return ((AbstractObject)selection.getFirstElement()).getObjectId();
   }

   /**
    * Change management status for selected objects
    *
    * @param managed true to manage objects
    */
   private void changeObjectManagementState(final boolean managed)
   {
      final Object[] objects = ((IStructuredSelection)selectionProvider.getSelection()).toArray();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Change object management status"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               if (o instanceof AbstractObject)
                  session.setObjectManaged(((AbstractObject)o).getObjectId(), managed);
               else if (o instanceof ObjectWrapper)
                  session.setObjectManaged(((ObjectWrapper)o).getObjectId(), managed);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change object management status");
         }
      }.start();
   }

   /**
    * Change maintenance state for selected objects
    *
    * @param enter true to enter maintenance
    */
   private void changeObjectMaintenanceState(final boolean enter)
   {
      final String comments;
      if (enter)
      {
         InputDialog dlg = new InputDialog(null, i18n.tr("Enter Maintenance"), i18n.tr("Additional comments"), "", null);
         if (dlg.open() != Window.OK)
            return;
         comments = dlg.getValue().trim();
      }
      else
      {
         comments = null;
      }

      final Object[] objects = ((IStructuredSelection)selectionProvider.getSelection()).toArray();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Change object maintenance state"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot change object maintenance state");
         }
      }.start();
   }

   /**
    * Schedule maintenance
    */
   private void scheduleMaintenance()
   {
      final MaintanenceScheduleDialog dialog = new MaintanenceScheduleDialog(view.getWindow().getShell());
      if (dialog.open() != Window.OK)
         return;

      final Object[] objects = ((IStructuredSelection)selectionProvider.getSelection()).toArray();
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Schedule maintenance"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               if (o instanceof AbstractObject)
               {
                  ScheduledTask taskStart = new ScheduledTask("Maintenance.Enter", "", "", dialog.getComments(), dialog.getStartTime(), ScheduledTask.SYSTEM, ((AbstractObject)o).getObjectId());
                  ScheduledTask taskEnd = new ScheduledTask("Maintenance.Leave", "", "", dialog.getComments(), dialog.getEndTime(), ScheduledTask.SYSTEM, ((AbstractObject)o).getObjectId());
                  session.addScheduledTask(taskStart);
                  session.addScheduledTask(taskEnd);
               }
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot schedule maintenance");
         }
      }.start();
   }
}
