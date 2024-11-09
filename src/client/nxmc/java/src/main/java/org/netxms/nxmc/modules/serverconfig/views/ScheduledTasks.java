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
package org.netxms.nxmc.modules.serverconfig.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.dialogs.ScheduledTaskEditDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ScheduledTaskComparator;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ScheduledTaskFilter;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ScheduledTaskLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Scheduled task manager
 */
public class ScheduledTasks extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ScheduledTasks.class);

   public static final int SCHEDULE_ID = 0;
   public static final int CALLBACK_ID = 1;
   public static final int OBJECT = 2;
   public static final int PARAMETERS = 3;
   public static final int TIMER_KEY = 4;
   public static final int EXECUTION_TIME = 5;
   public static final int EXECUTION_TIME_DESCRIPTION = 6;
   public static final int LAST_EXECUTION_TIME = 7;
   public static final int STATUS = 8;
   public static final int MANAGMENT_STATE = 9;
   public static final int OWNER = 10;
   public static final int COMMENTS = 11;

   private NXCSession session;
   private SessionListener listener;
   private SortableTableViewer viewer;
   private ScheduledTaskFilter filter;
   private Action actionShowCompletedTasks;
   private Action actionShowDisabledTasks;
   private Action actionShowSystemTasks;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionDisable;
   private Action actionEnable;
   private Action actionReschedule;

   /**
    * Create notification channels view
    */
   public ScheduledTasks()
   {
      super(LocalizationHelper.getI18n(ScheduledTasks.class).tr("Scheduled Tasks"), ResourceManager.getImageDescriptor("icons/config-views/scheduled-tasks.png"), "config.scheduled-tasks", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {      
      final int[] widths = { 50, 100, 200, 400, 200, 150, 200, 150, 100, 200, 250, 200 };
      final String[] names = { i18n.tr("ID"), i18n.tr("Schedule Type"), i18n.tr("Object"), i18n.tr("Parameters"), i18n.tr("Timer key"), i18n.tr("Execution time"), i18n.tr("Execution time description"), i18n.tr("Last execution time"),
            i18n.tr("Execution status"), i18n.tr("Administrative status"), i18n.tr("Owner"), i18n.tr("Comments") };
      viewer = new SortableTableViewer(parent, names, widths, SCHEDULE_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ScheduledTaskLabelProvider(viewer));
      viewer.setComparator(new ScheduledTaskComparator());
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

      filter = new ScheduledTaskFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "ScheduledTasks");
      final PreferenceStore settings = PreferenceStore.getInstance();
      filter.setShowCompletedTasks(settings.getAsBoolean("ScheduledTasks.showCompleted", true));
      filter.setShowDisabledTasks(settings.getAsBoolean("ScheduledTasks.showDisabled", true));
      filter.setShowSystemTasks(settings.getAsBoolean("ScheduledTasks.showSystem", false));

      viewer.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, "ScheduledTasks");
            settings.set("ScheduledTasks.showCompleted", filter.isShowCompletedTasks());
            settings.set("ScheduledTasks.showDisabled", filter.isShowDisabledTasks());
            settings.set("ScheduledTasks.showSystem", filter.isShowSystemTasks());
         }
      });

      createActions();
      createContextMenu();

      final Display display = parent.getDisplay();
      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.SCHEDULE_UPDATE)
            {
               display.asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     refresh();
                  }
               });
            }
         }
      };
      session.addListener(listener);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionShowCompletedTasks = new Action(i18n.tr("Show &completed tasks"), IAction.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            filter.setShowCompletedTasks(actionShowCompletedTasks.isChecked());
            viewer.refresh();
         }
      };
      actionShowCompletedTasks.setChecked(filter.isShowCompletedTasks());
      
      actionShowDisabledTasks = new Action(i18n.tr("Show &disabled tasks"), IAction.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            filter.setShowDisabledTasks(actionShowDisabledTasks.isChecked());
            viewer.refresh();
         }
      };
      actionShowDisabledTasks.setChecked(filter.isShowDisabledTasks());
      
      actionShowSystemTasks = new Action(i18n.tr("Show &system tasks"), IAction.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            filter.setShowSystemTasks(actionShowSystemTasks.isChecked());
            viewer.refresh();
         }
      };
      actionShowSystemTasks.setChecked(filter.isShowSystemTasks());

      actionCreate = new Action(i18n.tr("&New scheduled task..."), ResourceManager.getImageDescriptor("icons/add-task.png")) {
         @Override
         public void run()
         {
            createTask();
         }
      };
      addKeyBinding("M1+N", actionCreate);
      
      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editTask(false);
         }
      };
      addKeyBinding("M3+ENTER", actionEdit);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteTask();
         }
      };
      addKeyBinding("M1+D", actionDelete);

      actionDisable = new Action(i18n.tr("D&isable"), SharedIcons.DISABLE) {
         @Override
         public void run()
         {
            setScheduledTaskEnabled(false);
         }
      };
      addKeyBinding("M1+I", actionDisable);

      actionEnable = new Action(i18n.tr("En&able")) {
         @Override
         public void run()
         {
            setScheduledTaskEnabled(true);
         }
      };
      addKeyBinding("M1+E", actionEnable);

      actionReschedule = new Action(i18n.tr("&Reschedule..."), ResourceManager.getImageDescriptor("icons/reschedule-task.png")) {
         @Override
         public void run()
         {
            editTask(true);
         }
      };
      addKeyBinding("M1+R", actionReschedule);
   }

   /**
    * Delete task
    */
   protected void deleteTask()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Task"),
            (selection.size() == 1) ? i18n.tr("Selected task will be deleted. Are you sure?") : i18n.tr("Selected tasks will be deleted. Are you sure?")))
         return;

      new Job(i18n.tr("Delete scheduled task"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : selection.toList())
            {
               session.deleteScheduledTask(((ScheduledTask)o).getId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete scheduled tasks");
         }
      }.start();
   }

   /**
    * Edit selected task
    */
   protected void editTask(boolean reschedule)
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final ScheduledTask originalTask = (ScheduledTask)selection.getFirstElement();
      if (reschedule)
         originalTask.setId(0);

      new Job(i18n.tr("Updating scheduled task"), this) {
         private ScheduledTask task = null;

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<String> taskList = session.getScheduledTaskHandlers();
            getDisplay().syncExec(() -> {
               ScheduledTaskEditDialog dialog = new ScheduledTaskEditDialog(getWindow().getShell(), originalTask, taskList);
               if (dialog.open() == Window.OK)
               {
                  task = dialog.getTask();
               }
            });
            if (task != null)
            {
               if (reschedule)
                  session.addScheduledTask(task);
               else
                  session.updateScheduledTask(task);                  
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update scheduled tasks");
         }
      }.start();
   }

   /**
    * @param enabled
    */
   protected void setScheduledTaskEnabled(final boolean enabled)
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      new Job(i18n.tr("Updating scheduled task"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(Object o : selection.toList())
            {  
               ScheduledTask task = (ScheduledTask)o;
               task.setEnabed(enabled);
               session.updateScheduledTask(task);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update scheduled tasks");
         }
      }.start();
   }

   /**
    * Create task
    */
   private void createTask()
   {     
      new Job(i18n.tr("Creating scheduled task"), this) {
         private ScheduledTask task = null;
         
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<String> taskList = session.getScheduledTaskHandlers();
            getDisplay().syncExec(new Runnable() {
               @Override
               public void run()
               {
                  ScheduledTaskEditDialog dialog = new ScheduledTaskEditDialog(getWindow().getShell(), null, taskList);
                  if (dialog.open() == Window.OK)
                  {
                     task = dialog.getTask();
                  }
               }
            });
            if (task != null)
               session.addScheduledTask(task);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create scheduled tasks");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionShowCompletedTasks);
      manager.add(actionShowDisabledTasks);
      manager.add(actionShowSystemTasks);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreate);
   }
   
   /**
    * Create pop-up menu for variable list
    */
   private void createContextMenu()
   {
      MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = manager.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager mgr)
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() == 1) 
      {         
         ScheduledTask origin = (ScheduledTask)selection.toList().get(0);
         if (origin.getSchedule().isEmpty())
         {
            mgr.add(actionReschedule);
         }
         else
         {
            mgr.add(actionEdit);            
         }
      }

      boolean containDisabled = false;
      boolean containEnabled = false;
      for(Object o : selection.toList())
      {
         if(((ScheduledTask)o).isDisabled())
            containDisabled = true;
         else
            containEnabled = true;
         if(containDisabled && containEnabled)
            break;
      }
         
      if (containDisabled)
         mgr.add(actionEnable);
      if (containEnabled)
         mgr.add(actionDisable);

      if (selection.size() > 0)
         mgr.add(actionDelete);

      mgr.add(new Separator());
      mgr.add(actionCreate);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   public void refresh()
   {
      new Job(i18n.tr("Loading list of scheduled tasks"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<ScheduledTask> schedules = session.getScheduledTasks();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  final IStructuredSelection selection = viewer.getStructuredSelection();
                  viewer.setInput(schedules.toArray());
                  if (!selection.isEmpty())
                  {
                     List<ScheduledTask> newSelection = new ArrayList<ScheduledTask>();
                     for(Object o : selection.toList())
                     {
                        long id = ((ScheduledTask)o).getId();
                        for(ScheduledTask s : schedules)
                        {
                           if (s.getId() == id)
                           {
                              newSelection.add(s);
                              break;
                           }
                        }
                     }
                     if (!newSelection.isEmpty())
                        viewer.setSelection(new StructuredSelection(newSelection));
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of scheduled tasks");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();      
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if ((listener != null) && (session != null))
         session.removeListener(listener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
   }
}
