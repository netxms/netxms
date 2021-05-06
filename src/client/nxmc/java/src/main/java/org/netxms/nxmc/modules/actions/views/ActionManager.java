/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.FilterText;
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
 * Action manager view
 */
public class ActionManager extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(ActionManager.class);
   private static final String TABLE_CONFIG_PREFIX = "ActionList";

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
   private Action actionShowFilter;

   private ActionManagerFilter filter;
   private FilterText filterText;
   private Composite content;

   /**
    * Create action manager view
    */
   public ActionManager()
   {
      super(i18n.tr("Actions"), ResourceManager.getImageDescriptor("icons/config-views/actions.png"));
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      content = new Composite(parent, SWT.NONE);
      content.setLayout(new FormLayout());

      // Create filter area
      filterText = new FilterText(content, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });

      final String[] columnNames = { i18n.tr("Name"), i18n.tr("Type"), i18n.tr("Recipient"),
            i18n.tr("Subject"), i18n.tr("Data"), i18n.tr("Channel") };
      final int[] columnWidths = { 150, 90, 100, 120, 200, 100 };
      viewer = new SortableTableViewer(content, columnNames, columnWidths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
      WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      ActionLabelProvider labelProvider = new ActionLabelProvider();
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new ActionComparator());
      filter = new ActionManagerFilter(labelProvider);
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionEdit.setEnabled(selection.size() == 1);
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
            WidgetHelper.saveTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getTable().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      createActions();
      createPopupMenu();

      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(false);
         }
      });

      // Set initial focus to filter input line
      if (actionShowFilter.isChecked())
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly

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
      actionNew = new Action(i18n.tr("&Create..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createAction();
         }
      };

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editAction();
         }
      };

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteActions();
         }
      };

      actionShowFilter = new Action("Show &filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(PreferenceStore.getInstance().getAsBoolean("ActionManager.showFilter", true));
   }

   /**
    * Create pop-up menu for user list
    */
   private void createPopupMenu()
   {
      // Create menu manager
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      mgr.add(actionNew);
      mgr.add(actionEdit);
      mgr.add(actionDelete);
   }

   /**
    * Create new action
    */
   private void createAction()
   {
      final ServerAction action = new ServerAction(0);
      final EditActionDlg dlg = new EditActionDlg(getWindow().getShell(), action, true);
      if (dlg.open() == Window.OK)
      {
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
   }

   /**
    * Edit currently selected action
    */
   private void editAction()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;

      final ServerAction action = (ServerAction)selection.getFirstElement();
      final EditActionDlg dlg = new EditActionDlg(getWindow().getShell(), action, false);
      if (dlg.open() == Window.OK)
      {
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
   }

   /**
    * Delete selected actions
    */
   private void deleteActions()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
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

   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      filterText.setVisible(enable);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      content.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
      PreferenceStore.getInstance().set("ActionManager.showFilter", enable);
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }
}
