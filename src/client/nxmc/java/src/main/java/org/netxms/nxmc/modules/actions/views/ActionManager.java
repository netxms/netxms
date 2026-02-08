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
package org.netxms.nxmc.modules.actions.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
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
import org.netxms.client.ServerAction;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.actions.dialogs.EditActionDlg;
import org.netxms.nxmc.modules.actions.views.helpers.ActionComparator;
import org.netxms.nxmc.modules.actions.views.helpers.ActionLabelProvider;
import org.netxms.nxmc.modules.actions.views.helpers.ActionManagerFilter;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Action configuration view
 */
public class ActionManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(ActionManager.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_RCPT = 2;
   public static final int COLUMN_SUBJ = 3;
   public static final int COLUMN_DATA = 4;
   public static final int COLUMN_CHANNEL = 5;

   private SortableTableViewer viewer;
   private NXCSession session;
   private SessionListener sessionListener;
   private Map<Long, ServerAction> actions = new HashMap<Long, ServerAction>();
   private Action actionNew;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionClone;
   private Action actionEnable;
   private Action actionDisable;

   /**
    * Create action configuration view
    */
   public ActionManager()
   {
      super(LocalizationHelper.getI18n(ActionManager.class).tr("Actions"), ResourceManager.getImageDescriptor("icons/config-views/actions.png"), "configuration.actions", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      final String[] columnNames = { i18n.tr("Name"), i18n.tr("Type"), i18n.tr("Recipient"),
            i18n.tr("Subject"), i18n.tr("Data"), i18n.tr("Channel") };
      final int[] columnWidths = { 150, 90, 100, 120, 200, 100 };
      viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      WidgetHelper.restoreTableViewerSettings(viewer, "ActionManager");
      viewer.setContentProvider(new ArrayContentProvider());
      ActionLabelProvider labelProvider = new ActionLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new ActionComparator());
      ActionManagerFilter filter = new ActionManagerFilter(labelProvider);
      setFilterClient(viewer, filter);
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = event.getStructuredSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
               actionClone.setEnabled(selection.size() == 1);
               actionDelete.setEnabled(selection.size() > 0);
            }
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, "ActionManager");
         }
      });

      createActions();
      createContextMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case SessionNotification.ACTION_CREATED:
               case SessionNotification.ACTION_MODIFIED:
                  synchronized(actions)
                  {
                     actions.put(n.getSubCode(), (ServerAction)n.getObject());
                  }
                  updateActionsList();
                  break;
               case SessionNotification.ACTION_DELETED:
                  synchronized(actions)
                  {
                     actions.remove(n.getSubCode());
                  }
                  updateActionsList();
                  break;
            }
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
      new Job(i18n.tr("Load configured actions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<ServerAction> actions = session.getActions();
            synchronized(ActionManager.this.actions)
            {
               ActionManager.this.actions.clear();
               for(ServerAction a : actions)
               {
                  ActionManager.this.actions.put(a.getId(), a);
               }
            }

            updateActionsList();
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get configured actions");
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
            createAction();
         }
      };
      addKeyBinding("M1+N", actionNew);

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editAction();
         }
      };
      addKeyBinding("M3+ENTER", actionEdit);

      actionClone = new Action(i18n.tr("&Clone..."), SharedIcons.CLONE) {
         @Override
         public void run()
         {
            cloneAction();
         }
      };
      addKeyBinding("M1+M3+C", actionClone);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteActions();
         }
      };
      addKeyBinding("M1+D", actionDelete);

      actionEnable = new Action("En&able") {
         @Override
         public void run()
         {
            enableActions(true);
         }
      };
      addKeyBinding("M1+E", actionEnable);

      actionDisable = new Action("D&isable") {
         @Override
         public void run()
         {
            enableActions(false);
         }
      };
      addKeyBinding("M1+I", actionDisable);
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
      manager.add(actionClone);
      manager.add(actionEdit);
      manager.add(actionEnable);
      manager.add(actionDisable);
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
    * Create new action
    */
   private void createAction()
   {
      final ServerAction action = new ServerAction(0);
      final EditActionDlg dlg = new EditActionDlg(getWindow().getShell(), action, true);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Create action"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            long id = session.createAction(action.getName());
            action.setId(id);
            session.modifyAction(action);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create action");
         }
      }.start();
   }

   /**
    * Clone existing action
    */
   private void cloneAction()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final ServerAction action = new ServerAction(0, (ServerAction)selection.getFirstElement());
      action.setName("Copy of " + action.getName());
      final EditActionDlg dlg = new EditActionDlg(getWindow().getShell(), action, true);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Create action"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            long id = session.createAction(action.getName());
            action.setId(id);
            session.modifyAction(action);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create action");
         }
      }.start();
   }

   /**
    * Edit currently selected action
    */
   private void editAction()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final ServerAction action = (ServerAction)selection.getFirstElement();
      final EditActionDlg dlg = new EditActionDlg(getWindow().getShell(), action, false);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Update action configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyAction(action);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update action configuration");
         }
      }.start();
   }

   /**
    * Delete selected actions
    */
   private void deleteActions()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Delete"),
            i18n.tr("Do you really want to delete selected actions?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Delete actions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               session.deleteAction(((ServerAction)objects[i]).getId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete action");
         }
      }.start();
   }

   /**
    * Enable/disable selected actions
    * 
    * @param enable true to enable
    */
   private void enableActions(final boolean enable)
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Updating action status"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               ServerAction action = (ServerAction)objects[i];
               action.setDisabled(!enable);
               session.modifyAction(action);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot change action status");
         }
      }.start();
   }

   /**
    * Update actions list
    */
   private void updateActionsList()
   {
      viewer.getControl().getDisplay().asyncExec(new Runnable() {
         @Override
         public void run()
         {
            synchronized(ActionManager.this.actions)
            {
               viewer.setInput(ActionManager.this.actions.values().toArray());
            }
         }
      });
   }
}
