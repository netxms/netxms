/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.ai.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.ai.AiAgentTask;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.dialogs.AiTaskEditDialog;
import org.netxms.nxmc.modules.ai.views.helpers.AiTaskComparator;
import org.netxms.nxmc.modules.ai.views.helpers.AiTaskFilter;
import org.netxms.nxmc.modules.ai.views.helpers.AiTaskLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * AI task management view
 */
public class AiTaskManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(AiTaskManager.class);

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_DESCRIPTION = 1;
   public static final int COLUMN_OWNER = 2;
   public static final int COLUMN_STATE = 3;
   public static final int COLUMN_LAST_EXECUTION = 4;
   public static final int COLUMN_NEXT_EXECUTION = 5;
   public static final int COLUMN_EXPLANATION = 6;

   private SortableTableViewer viewer;
   private NXCSession session;
   private SessionListener sessionListener;
   private Map<Long, AiAgentTask> tasks = new HashMap<>();
   private Action actionNew;
   private Action actionDelete;

   /**
    * Create action configuration view
    */
   public AiTaskManager()
   {
      super(LocalizationHelper.getI18n(AiTaskManager.class).tr("AI Tasks"), ResourceManager.getImageDescriptor("icons/config-views/ai-tasks.png"), "configuration.ai-tasks", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] columnNames = { i18n.tr("ID"), i18n.tr("Description"), i18n.tr("Owner"), i18n.tr("State"), i18n.tr("Last Execution"), i18n.tr("Next Execution"), i18n.tr("Explanation") };
      final int[] columnWidths = { 90, 300, 150, 100, 150, 150, 400 };
      viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      WidgetHelper.restoreTableViewerSettings(viewer, "AITaskManager");
      viewer.setContentProvider(new ArrayContentProvider());
      AiTaskLabelProvider labelProvider = new AiTaskLabelProvider(viewer);
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new AiTaskComparator());
      AiTaskFilter filter = new AiTaskFilter(labelProvider);
      setFilterClient(viewer, filter);
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = event.getStructuredSelection();
            if (selection != null)
            {
               actionDelete.setEnabled(selection.size() > 0);
            }
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, "AITaskManager");
         }
      });

      createActions();
      createContextMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            // TODO: handle AI task related notifications
         }
      };
      session.addListener(sessionListener);
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

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      session.removeListener(sessionListener);
      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading AI tasks"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<AiAgentTask> tasks = session.getAiAgentTasks();
            synchronized(AiTaskManager.this.tasks)
            {
               AiTaskManager.this.tasks.clear();
               for(AiAgentTask t : tasks)
               {
                  AiTaskManager.this.tasks.put(t.getId(), t);
               }
            }

            updateTaskList();
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get list of AI tasks");
         }
      }.start();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionNew = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createTask();
         }
      };
      addKeyBinding("M1+N", actionNew);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteTasks();
         }
      };
      addKeyBinding("M1+D", actionDelete);
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillContextMenu(m));

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param manager Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionNew);
      manager.add(actionDelete);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNew);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionNew);
   }

   /**
    * Create new task
    */
   private void createTask()
   {
      final AiTaskEditDialog dlg = new AiTaskEditDialog(getWindow().getShell());
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Create action"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            long id = session.createAiAgentTask(dlg.getDescription(), dlg.getPrompt());
            runInUIThread(() -> {
               refresh(); // FIXME: should add only new task instead of full refresh or wait for update via notification
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create action");
         }
      }.start();
   }

   /**
    * Delete selected tasks
    */
   private void deleteTasks()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Delete"),
            i18n.tr("Do you really want to delete selected tasks?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Delete AI agent tasks"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               session.deleteAiAgentTask(((AiAgentTask)objects[i]).getId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete AI agent task");
         }
      }.start();
   }

   /**
    * Update task list
    */
   private void updateTaskList()
   {
      viewer.getControl().getDisplay().asyncExec(() -> {
         synchronized(AiTaskManager.this.tasks)
         {
            viewer.setInput(AiTaskManager.this.tasks.values().toArray());
         }
      });
   }
}
