/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
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
import org.netxms.client.constants.ServerVariableDataType;
import org.netxms.client.server.ServerVariable;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.dialogs.VariableEditDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ServerVariableComparator;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ServerVariablesFilter;
import org.netxms.nxmc.modules.serverconfig.views.helpers.ServerVariablesLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Server configuration variable view
 */
public class ServerVariables extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(ServerVariables.class);
   private static final String TABLE_CONFIG_PREFIX = "ServerVariables";

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_VALUE = 1;
   public static final int COLUMN_DEFAULT_VALUE = 2;
   public static final int COLUMN_NEED_RESTART = 3;
   public static final int COLUMN_DESCRIPTION = 4;

   private SortableTableViewer viewer;
   private NXCSession session;
   private Map<String, ServerVariable> varList;
   private Composite content;
   private FilterText filterText;
   private ServerVariablesFilter filter;

   private Action actionAdd;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionShowFilter;
   private Action actionDefaultValue;

   /**
    * Create server configuration variable view
    */
   public ServerVariables()
   {
      super(i18n.tr("Server configuration"), ResourceManager.getImageDescriptor("icons/config-views/server_config.png"));
      session = Registry.getSession();
   }

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

      final String[] names = { i18n.tr("Name"), i18n.tr("Value"), i18n.tr("Default value"), i18n.tr("Restart"),
            i18n.tr("Description") };
      final int[] widths = { 200, 150, 150, 80, 500 };
      viewer = new SortableTableViewer(content, names, widths, 0, SWT.FULL_SELECTION | SWT.MULTI | SWT.BORDER,
            SortableTableViewer.DEFAULT_STYLE);
      WidgetHelper.restoreTableViewerSettings(viewer, TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ServerVariablesLabelProvider());
      viewer.setComparator(new ServerVariableComparator());
      filter = new ServerVariablesFilter();
      viewer.addFilter(filter);
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editVariable();
         }
      });
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            actionEdit.setEnabled(selection.size() == 1);
            actionDelete.setEnabled(selection.size() > 0);
            actionDefaultValue.setEnabled(selection.size() > 0);
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

      filterText.setCloseAction(actionShowFilter);

      // Set initial focus to filter input line
      if (actionShowFilter.isChecked())
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
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
    * Create actions
    */
   private void createActions()
   {            
      actionAdd = new Action(i18n.tr("Create new..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            addVariable();
         }
      };
      actionAdd.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.add_config_variable"); 
      
      actionEdit = new Action(i18n.tr("Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editVariable();
         }
      };
      actionEdit.setEnabled(false);
      
      actionDelete = new Action(i18n.tr("Delete"), SharedIcons.DELETE_OBJECT) {
      @Override
         public void run()
         {
            deleteVariables();
         }
      };
      actionDelete.setEnabled(false);
      
      actionShowFilter = new Action("Show filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(PreferenceStore.getInstance().getAsBoolean(TABLE_CONFIG_PREFIX + "showFilter", true));
      
      actionDefaultValue = new Action("Set default value") {
         @Override
         public void run()
         {
            setDefaultValue();
         }
      };
      
      actionExportToCsv = new ExportToCsvAction(this, viewer, true);
      actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
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
   }

   /**
    * Fill context menu
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager mgr)
   {
      mgr.add(actionAdd);
      mgr.add(actionEdit);
      mgr.add(actionDelete);
      mgr.add(actionDefaultValue);
      mgr.add(new Separator());
      mgr.add(actionExportToCsv);
      mgr.add(actionExportAllToCsv);
   }
   
   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Load server configuration variables"), this) {

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load server configuration variables");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            varList = session.getServerVariables();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  synchronized(varList)
                  {
                     viewer.setInput(varList.values().toArray());
                  }
               }
            });
         }
      }.start();
   }

   /**
    * Add new variable
    */
   private void addVariable()
   {
      final VariableEditDialog dlg = new VariableEditDialog(getWindow().getShell(),
            new ServerVariable(null, "", false, ServerVariableDataType.STRING, "", ""));
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Create configuration variable"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.setServerVariable(dlg.getVarName(), dlg.getVarValue());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     refresh();
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot create configuration variable");
            }
         }.start();
      }
   }

   /**
    * Edit currently selected variable
    */
   private void editVariable()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection == null) || (selection.size() != 1))
         return;

      final ServerVariable var = (ServerVariable)selection.getFirstElement();
      final VariableEditDialog dlg = new VariableEditDialog(getWindow().getShell(), var);
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Modify configuration variable"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.setServerVariable(dlg.getVarName(), dlg.getVarValue());
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     refresh();
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot modify configuration variable");
            }
         }.start();
      }
   }

   /**
    * Delete selected variables
    */
   private void deleteVariables()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection == null) || (selection.size() == 0))
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Delete"),
            i18n.tr("Are you sure you want to delete selected configuration variables?")))
         return;

      final List<String> names = new ArrayList<String>(selection.size());
      for(Object o : selection.toList())
      {
         if (o instanceof ServerVariable)
            names.add(((ServerVariable)o).getName());
      }
      new Job(i18n.tr("Delete configuration variables"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(String n : names)
            {
               session.deleteServerVariable(n);
            }
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete configuration variable");
         }
      }.start();
   }

   /**
    * Reset variable values to default
    */
   @SuppressWarnings("unchecked")
   private void setDefaultValue()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      final List<ServerVariable> list = selection.toList();

      new Job(i18n.tr("Set default server config values"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.setDefaultServerValues(list);
            varList = session.getServerVariables();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  synchronized(varList)
                  {
                     viewer.setInput(varList.values().toArray());
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Error setting default server config values");
         }
      }.start();
   }

   @Override
   public boolean isModified()
   {
      return false;
   }

   @Override
   public void save()
   {
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
      PreferenceStore.getInstance().set(TABLE_CONFIG_PREFIX + "showFilter", enable);
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
