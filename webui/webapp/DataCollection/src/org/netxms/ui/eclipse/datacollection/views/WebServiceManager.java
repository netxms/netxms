/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.ui.eclipse.datacollection.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
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
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.pages.WebServiceGeneral;
import org.netxms.ui.eclipse.datacollection.dialogs.pages.WebServiceHeaders;
import org.netxms.ui.eclipse.datacollection.views.helpers.WebServiceDefinitionFilter;
import org.netxms.ui.eclipse.datacollection.views.helpers.WebServiceDefinitionLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Web service manager
 */
public class WebServiceManager extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.datacollection.views.WebServiceManager"; //$NON-NLS-1$

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_URL = 1;
   public static final int COLUMN_AUTHENTICATION = 2;
   public static final int COLUMN_LOGIN = 3;
   public static final int COLUMN_RETENTION_TIME = 4;
   public static final int COLUMN_TIMEOUT = 5;
   public static final int COLUMN_DESCRIPTION = 6;

   private Map<Integer, WebServiceDefinition> webServiceDefinitions = new HashMap<Integer, WebServiceDefinition>();
   private int nameCounter = 1;
   private Composite content;
   private FilterText filterText;
   private WebServiceDefinitionFilter filter;
   private SortableTableViewer viewer;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;
   private Action actionShowFilter;
   private RefreshAction actionRefresh;

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
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
      filterText.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
            actionShowFilter.setChecked(false);
         }
      });

      final String[] names = { "Name", "URL", "Authentication", "Login", "Retention Time", "Timeout", "Description" };
      final int[] widths = { 300, 600, 100, 160, 90, 90, 600 };
      viewer = new SortableTableViewer(content, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new WebServiceDefinitionLabelProvider());
      filter = new WebServiceDefinitionFilter();
      viewer.addFilter(filter);
      WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "WebServiceManager"); //$NON-NLS-1$
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            actionEdit.setEnabled(selection.size() == 1);
            actionDelete.setEnabled(!selection.isEmpty());
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editDefinition();
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "WebServiceManager"); //$NON-NLS-1$
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
      contributeToActionBars();
      createPopupMenu();
      activateContext();
      refresh();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.datacollection.context.WebServiceManager"); //$NON-NLS-1$
      }
   }

   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(actionCreate);
      manager.add(actionEdit);
      manager.add(actionDelete);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionCreate);
      manager.add(new Separator());
      manager.add(actionShowFilter);
      manager.add(actionRefresh);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(actionCreate);
      manager.add(actionEdit);
      manager.add(actionDelete);
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
      getSite().setSelectionProvider(viewer);
      getSite().registerContextMenu(menuMgr, viewer);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);

      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            refresh();
         }
      };

      actionCreate = new Action("&New...", SharedIcons.ADD_OBJECT) { //$NON-NLS-1$
         @Override
         public void run()
         {
            createDefinition();
         }
      };

      actionEdit = new Action(Messages.get().DataCollectionEditor_ActionEdit, SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editDefinition();
         }
      };
      actionEdit.setText(Messages.get().DataCollectionEditor_Edit);
      actionEdit.setImageDescriptor(Activator.getImageDescriptor("icons/edit.png")); //$NON-NLS-1$
      actionEdit.setEnabled(false);

      actionDelete = new Action(Messages.get().DataCollectionEditor_Delete, SharedIcons.DELETE_OBJECT) { // $NON-NLS-1$
         @Override
         public void run()
         {
            deleteDefinitions();
         }
      };
      actionDelete.setEnabled(false);

      actionShowFilter = new Action(Messages.get().DataCollectionEditor_ShowFilter, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(getBooleanFromSettings("WebServiceManager.showFilter", true));
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.datacollection.commands.show_websvc_filter"); //$NON-NLS-1$
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
   }

   /**
    * Get boolean value from dialog settings
    * 
    * @param name
    * @param defval
    * @return
    */
   private boolean getBooleanFromSettings(String name, boolean defval)
   {
      String v = Activator.getDefault().getDialogSettings().get(name);
      return (v != null) ? Boolean.valueOf(v) : defval;
   }

   /**
    * Refresh list
    */
   private void refresh()
   {
      final NXCSession session = ConsoleSharedData.getSession();
      ConsoleJob job = new ConsoleJob("Get web service definitions", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<WebServiceDefinition> definitions = session.getWebServiceDefinitions();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  webServiceDefinitions.clear();
                  for(WebServiceDefinition d : definitions)
                     webServiceDefinitions.put(d.getId(), d);
                  viewer.setInput(webServiceDefinitions.values());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get web service definitions";
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Create web service definition
    */
   private void createDefinition()
   {
      final WebServiceDefinition definition = new WebServiceDefinition("Web Service " + Integer.toString(nameCounter++));
      if (showDefinitionEditDialog(definition))
      {
         final NXCSession session = ConsoleSharedData.getSession();
         new ConsoleJob("Create web service definition", this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               definition.setId(session.modifyWebServiceDefinition(definition));
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     webServiceDefinitions.put(definition.getId(), definition);
                     viewer.refresh();
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot create web service definition";
            }
         }.start();
      }
   }

   /**
    * Edit selected web service definition
    */
   private void editDefinition()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;

      final WebServiceDefinition definition = (WebServiceDefinition)selection.getFirstElement();
      if (showDefinitionEditDialog(definition))
      {
         final NXCSession session = ConsoleSharedData.getSession();
         new ConsoleJob("Modify web service definition", this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.modifyWebServiceDefinition(definition);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     viewer.refresh();
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return "Cannot update web service definition";
            }
         }.start();
      }
   }

   /**
    * Show web service definitoin configuration dialog
    * 
    * @param definition web service defninition object
    * @return true if OK was pressed
    */
   private boolean showDefinitionEditDialog(WebServiceDefinition definition)
   {
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("general", new WebServiceGeneral(definition)));
      pm.addToRoot(new PreferenceNode("headers", new WebServiceHeaders(definition)));

      PreferenceDialog dlg = new PreferenceDialog(getViewSite().getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText("Edit Web Service Definition");
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
   }

   /**
    * Delete selected web service definitions
    */
   private void deleteDefinitions()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Delete Web Service Definitions",
            "Selected web service definitions will be permanently deleted. Are you sure?"))
         return;

      final int[] deleteList = new int[selection.size()];
      int index = 0;
      for(Object o : selection.toList())
         deleteList[index++] = ((WebServiceDefinition)o).getId();

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Delete web service definitions", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            for(int id : deleteList)
               session.deleteWebServiceDefinition(id);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  for(int id : deleteList)
                     webServiceDefinitions.remove(id);
                  viewer.refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete web service definition";
         }
      }.start();
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
      Activator.getDefault().getDialogSettings().put("WebServiceManager.showFilter", enable);
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
