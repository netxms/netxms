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
package org.netxms.nxmc.modules.objects;

import java.util.ArrayList;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.helpers.TimePeriodFunctions;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.MaintanenceScheduleDialog;
import org.xnap.commons.i18n.I18n;

/**
 * Maintenance menu manager
 */
public class MaintenanceMenuManager extends MenuManager
{
   private final I18n i18n = LocalizationHelper.getI18n(MaintenanceMenuManager.class);

   private View view;   
   private ISelectionProvider selectionProvider;

   private Action actionEnterMaintenance;
   private Action actionLeaveMaintenance;
   private Action actionScheduleMaintenance;
   private ArrayList<Action> actionMaintenance;

   /**
    * Create new menu manager for object's "Maintenance" menu.
    *
    * @param shell parent shell
    * @param view parent view
    * @param selection object to create menu for
    */
   public MaintenanceMenuManager(View view, ISelectionProvider selectionProvider)
   {
      this.view = view;
      this.selectionProvider = selectionProvider;
      
      setMenuText(i18n.tr("M&aintenance"));  

      createActions();
      add(actionEnterMaintenance);
      add(actionLeaveMaintenance);
      add(new Separator());
      for (Action action : actionMaintenance)
      {
         add(action);
      }
      add(new Separator());
      add(actionScheduleMaintenance);      
   }
   

   /**
    * Create object actions
    */
   protected void createActions()
   {      
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
      
      initializeTimeMaintenance();
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

   /**
    * Initialize predefined object maintenance periods
    */
   private void initializeTimeMaintenance()
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      int menuSize = settings.getAsInteger("Maintenance.TimeMenuSize", 0);
      actionMaintenance = new ArrayList<Action>(menuSize);
      for(int i = 0; i < menuSize; i++)
      {
         final int time = settings.getAsInteger("Maintenance.TimeMenuEntry." + Integer.toString(i), 0); 
         if (time == 0)
            continue;
         actionMaintenance.add(createTimeMaintenanceAction(time, i));
      }
   }

   /**
    * Create action for time acknowledge.
    * 
    * @param time time in seconds
    * @param index action index
    * @return new action
    */
   private Action createTimeMaintenanceAction(int time, int index)
   {
      Action action = new Action(TimePeriodFunctions.timeToString(time)) {
         @Override
         public void run()
         {
            scheduleMaintenance(time);
         }
      };
      action.setId("Maintenance.TimeMenuEntry." + Integer.toString(index));
      return action;
   }

   /**
    * Schedule maintenance
    */
   private void scheduleMaintenance(int time)
   {
      InputDialog dlg = new InputDialog(null, i18n.tr("Enter Maintenance"), i18n.tr("Additional comments"), "", null);
      if (dlg.open() != Window.OK)
         return;
      final String comments = dlg.getValue().trim();

      final Object[] objects = ((IStructuredSelection)selectionProvider.getSelection()).toArray();
      final NXCSession session = Registry.getSession();
      long timestamp = System.currentTimeMillis() + time * 1000;
      new Job(i18n.tr("Schedule maintenance"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : objects)
            {
               long objectId = 0;
               if (o instanceof AbstractObject)
               {
                  objectId = ((AbstractObject)o).getObjectId();
               }
               else if (o instanceof ObjectWrapper)
               {
                  objectId = ((ObjectWrapper)o).getObjectId();
               }
               if (objectId != 0)
               {
                  session.setObjectMaintenanceMode(objectId, true, comments);
                  ScheduledTask taskEnd = new ScheduledTask("Maintenance.Leave", "", "", comments, new Date(timestamp), ScheduledTask.SYSTEM, objectId);
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
