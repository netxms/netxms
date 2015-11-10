package org.netxms.ui.eclipse.serverconfig.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.dialogs.RerunTimeDialog;
import org.netxms.ui.eclipse.serverconfig.dialogs.ScheduledTaskEditor;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ScheduleTableEntryComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ScheduleTableEntryLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

public class ScheduledTaskView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.ScheduledTaskView"; //$NON-NLS-1$

   public static final int SCHEDULE_ID = 0;
   public static final int CALLBACK_ID = 1;
   public static final int OBJECT = 2;
   public static final int PARAMETERS = 3;
   public static final int EXECUTION_TIME = 4;
   public static final int LAST_EXECUTION_TIME = 5;
   public static final int STATUS = 6;   
   public static final int OWNER = 7;
   

   private NXCSession session;
   private SessionListener listener;
   private SortableTableViewer viewer;
   private Action actionRefresh;
   private Action actionNewScheduledTask;
   private Action actionEditScheduledTask;
   private Action actionDeleteScheduledTask;
   private Action actionDisbaleScheduledTask;
   private Action actionEnableScheduledTask;
   private Action actionReRun;
   
   
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
   }

   @Override
   public void createPartControl(Composite parent)
   {      
      final int[] widths = { 50, 100, 200, 400, 300, 300, 100, 100 };
      final String[] names = { "Id", "Schedule Type", "Object", "Parameters", "Execution time", "Last execution time", "Status", "Owner" };
      viewer = new SortableTableViewer(parent, names, widths, SCHEDULE_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ScheduleTableEntryLabelProvider());
      viewer.setComparator(new ScheduleTableEntryComparator());      

      createActions();
      contributeToActionBars();
      createPopupMenu();
      refresh();
      
      final Display display = getSite().getShell().getDisplay();
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

   private void createActions()
   {
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };
      
      actionNewScheduledTask = new Action("New scheduled task...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createNewScheduledTask();
         }
      };
      
      actionEditScheduledTask = new Action("Edit", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editScheduledTask();
         }
      };
      
      actionDeleteScheduledTask = new Action("Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteScheduledTask();
         }
      };
      
      actionDisbaleScheduledTask = new Action("Disable") {
         @Override
         public void run()
         {
            setScheduledTaskEnabled(false);
         }
      };
      
      actionEnableScheduledTask = new Action("Enable") {
         @Override
         public void run()
         {
            setScheduledTaskEnabled(true);
         }
      };
      
      actionReRun = new Action("Rerun", SharedIcons.EXECUTE) 
      {
         @Override
         public void run()
         {
            rerun();
         }
      };
   }

   protected void rerun()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();      
      if (selection.size() != 1)
         return;
      
      final ScheduledTask origin = (ScheduledTask)selection.toList().get(0);
      
      final RerunTimeDialog dialog = new RerunTimeDialog(getSite().getShell(), origin.getExecutionTime());
      
      if (dialog.open() != Window.OK)
         return;
      
      new ConsoleJob("Rerun scheduled task", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            origin.setExecutionTime(dialog.getRerunDate());
            origin.setFlags(origin.getFlags() & ~ScheduledTask.EXECUTED);
            session.updateSchedule(origin);            
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot rerun scheduled tasks";
         }
      }.start();
      
}

   protected void deleteScheduledTask()
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      
      if (selection.size() == 0)
         return;
      
      new ConsoleJob("Delete scheduled task", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(Object o : selection.toList())
            {
               session.removeSchedule(((ScheduledTask)o).getId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete scheduled tasks";
         }
      }.start();
   }

   protected void editScheduledTask()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();      
      if (selection.size() != 1)
         return;
      
      final ScheduledTask origin = (ScheduledTask)selection.toList().get(0);
      
      new ConsoleJob("Update scheduled task", null, Activator.PLUGIN_ID, null) {
         private ScheduledTask task = null;
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<String> taskList = session.listScheduleCallbacks();
            getDisplay().syncExec(new Runnable() {
               @Override
               public void run()
               {
                  ScheduledTaskEditor dialog = new ScheduledTaskEditor(getSite().getShell(), origin, taskList);
                  if (dialog.open() == Window.OK)
                  {
                     task = dialog.getScheduledTask();
                  }
               }
            });
            if(task != null)
               session.updateSchedule(task);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update scheduled tasks";
         }
      }.start();
   }

   protected void setScheduledTaskEnabled(final boolean enabled)
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();      
      if (selection.size() < 0)
         return;
      
      new ConsoleJob("Update scheduled task", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(Object o : selection.toList())
            {  
               ScheduledTask task = (ScheduledTask)o;
               task.setEnabed(enabled);
               session.updateSchedule(task);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update scheduled tasks";
         }
      }.start();
   }

   private void createNewScheduledTask()
   {     
      new ConsoleJob("Create scheduled task", null, Activator.PLUGIN_ID, null) {
         private ScheduledTask task = null;
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<String> taskList = session.listScheduleCallbacks();
            getDisplay().syncExec(new Runnable() {
               @Override
               public void run()
               {
                  ScheduledTaskEditor dialog = new ScheduledTaskEditor(getSite().getShell(), null, taskList);
                  if (dialog.open() == Window.OK)
                  {
                     task = dialog.getScheduledTask();
                  }
               }
            });
            if(task != null)
               session.addSchedule(task);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot create scheduled tasks";
         }
      }.start();
   }
   
   

   /**
    * Fill action bars
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * @param manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionNewScheduledTask);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNewScheduledTask);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
   
   /**
    * Create pop-up menu for variable list
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);

      // Register menu for extension.
      getSite().registerContextMenu(menuMgr, viewer);
   }
   
   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager mgr)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      mgr.add(actionNewScheduledTask);
      
      if (selection.size() == 1) 
      {         
         mgr.add(actionEditScheduledTask);
         ScheduledTask origin = (ScheduledTask)selection.toList().get(0);
         if(origin.getSchedule().isEmpty())
            mgr.add(actionReRun);
      }

      if (selection.size() > 0)
         mgr.add(actionDeleteScheduledTask);
      
      boolean containDisabled = false;
      boolean containEnabled = false;
      for(Object o : selection.toList())
      {
         if(((ScheduledTask)o).isDisbaled())
            containDisabled = true;
         else
            containEnabled = true;
         if(containDisabled && containEnabled)
            break;
      }
         
      if(containDisabled)
         mgr.add(actionEnableScheduledTask);
      if(containEnabled)
         mgr.add(actionDisbaleScheduledTask);
   }

   private void refresh()
   {
      new ConsoleJob(Messages.get().MappingTables_ReloadJobName, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<ScheduledTask> schedules = session.listScheduleTasks();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(schedules.toArray());
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return Messages.get().MappingTables_ReloadJobError;
         }
      }.start();
      
   }

   @Override
   public void setFocus()
   {
      viewer.getTable().setFocus();      
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      if ((listener != null) && (session != null))
         session.removeListener(listener);
      super.dispose();
   }
}
