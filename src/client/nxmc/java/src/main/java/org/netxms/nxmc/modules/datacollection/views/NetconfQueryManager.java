/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
import org.eclipse.jface.action.Separator;
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.NetconfQueryDefinition;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyDialog;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.propertypages.NetconfQueryGeneral;
import org.netxms.nxmc.modules.datacollection.views.helpers.NetconfQueryComparator;
import org.netxms.nxmc.modules.datacollection.views.helpers.NetconfQueryFilter;
import org.netxms.nxmc.modules.datacollection.views.helpers.NetconfQueryLabelProvider;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * NETCONF query manager
 */
public class NetconfQueryManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(NetconfQueryManager.class);

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_NAME = 1;
   public static final int COLUMN_DATASTORE = 2;
   public static final int COLUMN_FILTER_TYPE = 3;
   public static final int COLUMN_RETENTION_TIME = 4;
   public static final int COLUMN_TIMEOUT = 5;
   public static final int COLUMN_DESCRIPTION = 6;

   private Map<Integer, NetconfQueryDefinition> queryDefinitions = new HashMap<Integer, NetconfQueryDefinition>();
   private int nameCounter = 1;
   private NetconfQueryFilter filter;
   private SortableTableViewer viewer;
   private Action actionCreate;
   private Action actionEdit;
   private Action actionDelete;

   /**
    * Create NETCONF query manager view
    */
   public NetconfQueryManager()
   {
      super(LocalizationHelper.getI18n(NetconfQueryManager.class).tr("NETCONF Queries"), SharedIcons.XML, "configuration.netconf-queries", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      final String[] names = { i18n.tr("ID"), i18n.tr("Name"), i18n.tr("Datastore"), i18n.tr("Filter type"), i18n.tr("Retention time"), i18n.tr("Timeout"), i18n.tr("Description") };
      final int[] widths = { 60, 300, 120, 100, 90, 90, 600 };
      viewer = new SortableTableViewer(parent, names, widths, 1, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION, "NetconfQueryManager");
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new NetconfQueryLabelProvider());
      viewer.setComparator(new NetconfQueryComparator());
      filter = new NetconfQueryFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

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
      Action resetAction = viewer.getResetColumnOrderAction();
      if (resetAction != null)
         manager.add(resetAction);
      Action showAllAction = viewer.getShowAllColumnsAction();
      if (showAllAction != null)
         manager.add(showAllAction);
      Action autoSizeAction = viewer.getAutoSizeColumnsAction();
      if (autoSizeAction != null)
      {
         manager.add(new Separator());
         manager.add(autoSizeAction);
      }
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
    * Create context menu for NETCONF query definition list
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
      Job job = new Job(i18n.tr("Loading NETCONF query definitions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<NetconfQueryDefinition> definitions = session.getNetconfQueryDefinitions();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  queryDefinitions.clear();
                  for(NetconfQueryDefinition d : definitions)
                     queryDefinitions.put(d.getId(), d);
                  viewer.setInput(queryDefinitions.values());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get NETCONF query definitions");
         }
      };
      job.start();
   }

   /**
    * Create NETCONF query definition
    */
   private void createDefinition()
   {
      final NetconfQueryDefinition definition = new NetconfQueryDefinition("NETCONF Query " + Integer.toString(nameCounter++));
      if (showDefinitionEditDialog(definition))
      {
         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Creating NETCONF query definition"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               definition.setId(session.modifyNetconfQueryDefinition(definition));
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     queryDefinitions.put(definition.getId(), definition);
                     viewer.refresh();
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot create NETCONF query definition");
            }
         }.start();
      }
   }

   /**
    * Edit selected NETCONF query definition
    */
   private void editDefinition()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final NetconfQueryDefinition definition = (NetconfQueryDefinition)selection.getFirstElement();
      if (showDefinitionEditDialog(definition))
      {
         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Updating NETCONF query definition"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.modifyNetconfQueryDefinition(definition);
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
               return i18n.tr("Cannot update NETCONF query definition");
            }
         }.start();
      }
   }

   /**
    * Show NETCONF query definition configuration dialog
    *
    * @param definition NETCONF query definition object
    * @return true if OK was pressed
    */
   private boolean showDefinitionEditDialog(NetconfQueryDefinition definition)
   {
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("general", new NetconfQueryGeneral(definition)));
      PropertyDialog dlg = new PropertyDialog(getWindow().getShell(), pm, i18n.tr("Edit NETCONF Query Definition"));
      return dlg.open() == Window.OK;
   }

   /**
    * Delete selected NETCONF query definitions
    */
   private void deleteDefinitions()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete NETCONF Query Definitions"), i18n.tr("Selected NETCONF query definitions will be permanently deleted. Are you sure?")))
         return;

      final int[] deleteList = new int[selection.size()];
      int index = 0;
      for(Object o : selection.toList())
         deleteList[index++] = ((NetconfQueryDefinition)o).getId();

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Deleting NETCONF query definitions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int id : deleteList)
               session.deleteNetconfQueryDefinition(id);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  for(int id : deleteList)
                     queryDefinitions.remove(id);
                  viewer.refresh();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete NETCONF query definition");
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
