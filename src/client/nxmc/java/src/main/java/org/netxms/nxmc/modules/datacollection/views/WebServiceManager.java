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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyDialog;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.propertypages.WebServiceGeneral;
import org.netxms.nxmc.modules.datacollection.propertypages.WebServiceHeaders;
import org.netxms.nxmc.modules.datacollection.views.helpers.WebServiceDefinitionComparator;
import org.netxms.nxmc.modules.datacollection.views.helpers.WebServiceDefinitionFilter;
import org.netxms.nxmc.modules.datacollection.views.helpers.WebServiceDefinitionLabelProvider;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Web service manager
 */
public class WebServiceManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(WebServiceManager.class);

   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_URL = 1;
   public static final int COLUMN_METHOD = 2;
   public static final int COLUMN_AUTHENTICATION = 3;
   public static final int COLUMN_LOGIN = 4;
   public static final int COLUMN_RETENTION_TIME = 5;
   public static final int COLUMN_TIMEOUT = 6;
   public static final int COLUMN_DESCRIPTION = 7;

   private Map<Integer, WebServiceDefinition> webServiceDefinitions = new HashMap<Integer, WebServiceDefinition>();
   private int nameCounter = 1;
   private WebServiceDefinitionFilter filter;
   private SortableTableViewer viewer;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;

   /**
    * Create web service manager view
    */
   public WebServiceManager()
   {
      super(LocalizationHelper.getI18n(WebServiceManager.class).tr("Web Service Definitions"), SharedIcons.URL, "configuration.web-services", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      final String[] names = { i18n.tr("Name"), i18n.tr("URL"), i18n.tr("Method"), i18n.tr("Authentication"), i18n.tr("Login"), i18n.tr("Retention Time"), i18n.tr("Timeout"), i18n.tr("Description") };
      final int[] widths = { 300, 600, 100, 100, 160, 90, 90, 600 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new WebServiceDefinitionLabelProvider());
      viewer.setComparator(new WebServiceDefinitionComparator());
      filter = new WebServiceDefinitionFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      WidgetHelper.restoreTableViewerSettings(viewer, "WebServiceManager");
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
            WidgetHelper.saveTableViewerSettings(viewer, "WebServiceManager");
         }
      });

      createActions();
      createContextMenu();
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
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      viewer.getControl().setFocus();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCreate = new Action(i18n.tr("&New..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createDefinition();
         }
      };
      addKeyBinding("M1+N", actionCreate);

      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editDefinition();
         }
      };
      actionEdit.setEnabled(false);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteDefinitions();
         }
      };
      actionDelete.setEnabled(false);
      addKeyBinding("M1+D", actionDelete);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionCreate);
      manager.add(actionEdit);
      manager.add(actionDelete);
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
    * Create context menu for web service definition list
    */
   private void createContextMenu()
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
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Loading web service definitions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot get web service definitions");
         }
      };
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
         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Creating web service definition"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
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
               return i18n.tr("Cannot create web service definition");
            }
         }.start();
      }
   }

   /**
    * Edit selected web service definition
    */
   private void editDefinition()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final WebServiceDefinition definition = (WebServiceDefinition)selection.getFirstElement();
      if (showDefinitionEditDialog(definition))
      {
         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Updating web service definition"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
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
               return i18n.tr("Cannot update web service definition");
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
      PropertyDialog dlg = new PropertyDialog(getWindow().getShell(), pm, i18n.tr("Edit Web Service Definition"));
      return dlg.open() == Window.OK;
   }

   /**
    * Delete selected web service definitions
    */
   private void deleteDefinitions()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Web Service Definitions"), i18n.tr("Selected web service definitions will be permanently deleted. Are you sure?")))
         return;

      final int[] deleteList = new int[selection.size()];
      int index = 0;
      for(Object o : selection.toList())
         deleteList[index++] = ((WebServiceDefinition)o).getId();

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Deleting web service definitions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot delete web service definition");
         }
      }.start();
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
