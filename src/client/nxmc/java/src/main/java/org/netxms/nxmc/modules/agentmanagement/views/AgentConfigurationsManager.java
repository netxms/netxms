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
package org.netxms.nxmc.modules.agentmanagement.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.agent.config.AgentConfiguration;
import org.netxms.client.agent.config.AgentConfigurationHandle;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Agents configuration manager
 */
public class AgentConfigurationsManager extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(AgentConfigurationsManager.class);

   private NXCSession session;
   private TableViewer viewer;
   private Action actionNew;
   private Action actionEdit;
   private Action actionRename;
   private Action actionDelete;
   private Action actionMoveUp;
   private Action actionMoveDown;
   private List<AgentConfigurationHandle> elements;

   /**
    * Create server stored agent configuration  view
    */
   public AgentConfigurationsManager()
   {
      super(LocalizationHelper.getI18n(AgentConfigurationsManager.class).tr("Agent Configurations"), ResourceManager.getImageDescriptor("icons/config-views/tunnel_manager.png"), "AgentConfigurationsManager", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      viewer = new TableViewer(parent, SWT.FULL_SELECTION | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((AgentConfigurationHandle)element).getName();
         }
      });
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            actionEdit.setEnabled(selection.size() == 1);
            actionRename.setEnabled(selection.size() == 1);
            actionMoveUp.setEnabled(selection.size() == 1);
            actionMoveDown.setEnabled(selection.size() == 1);
            actionDelete.setEnabled(!selection.isEmpty());
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editConfig();
         }
      });

      ConfigurationFilter filter = new ConfigurationFilter();
      viewer.addFilter(filter);
      setFilterClient(viewer, filter);

      createActions();
      createPopupMenu();
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
    * Create context menu for configuration list
    */
   private void createPopupMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            mgr.add(actionNew);
            mgr.add(new Separator());
            mgr.add(actionEdit);
            mgr.add(actionRename);
            mgr.add(actionMoveUp);
            mgr.add(actionMoveDown);
            mgr.add(new Separator());
            mgr.add(actionDelete);
         }
      });

      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
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
            createNewConfig();
         }
      };
      addKeyBinding("M1+N", actionNew);

      actionEdit = new Action(i18n.tr("&Edit"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editConfig();
         }
      };

      actionRename = new Action(i18n.tr("&Rename...")) {
         @Override
         public void run()
         {
            renameConfig();
         }
      };
      addKeyBinding("F2", actionRename);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteConfig();
         }
      };
      addKeyBinding("M1+D", actionDelete);

      actionMoveUp = new Action(i18n.tr("Move &up"), SharedIcons.UP) {
         @Override
         public void run()
         {
            moveUp();
         }
      };

      actionMoveDown = new Action(i18n.tr("Move d&own"), SharedIcons.DOWN) {
         @Override
         public void run()
         {
            moveDown();
         }
      };
   }

   /**
    * Move selected item up
    */
   private void moveUp()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final AgentConfigurationHandle element1 = (AgentConfigurationHandle)selection.getFirstElement();

      int index = elements.indexOf(element1);
      if (index <= 0)
         return;

      final AgentConfigurationHandle element2 = elements.get(index - 1);

      new Job(i18n.tr("Move configuration up"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.swapAgentConfigs(element1.getId(), element2.getId());
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
            return i18n.tr("Cannot move configuration");
         }
      }.start();
   }

   /**
    * Move selected item down
    */
   private void moveDown()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final AgentConfigurationHandle element1 = (AgentConfigurationHandle)selection.getFirstElement();

      int index = elements.indexOf(element1);
      if (index >= (elements.size() - 1))
         return;

      final AgentConfigurationHandle element2 = elements.get(index + 1);

      new Job(i18n.tr("Move configuration down"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.swapAgentConfigs(element1.getId(), element2.getId());
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
            return i18n.tr("Cannot move configuration");
         }
      }.start();
   }

   /**
    * Edit selected item
    */
   private void editConfig()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final AgentConfigurationHandle h = (AgentConfigurationHandle)selection.getFirstElement();
      openView(new StoredAgentConfigurationEditor(h.getId(), h.getName()));
   }

   /**
    * Rename selected item
    */
   private void renameConfig()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final AgentConfigurationHandle h = (AgentConfigurationHandle)selection.getFirstElement();

      InputDialog dlg = new InputDialog(getWindow().getShell(), i18n.tr("Rename Configuration"), i18n.tr("Configuration name"), h.getName(), new IInputValidator() {
         @Override
         public String isValid(String newText)
         {
            return newText.isBlank() ? i18n.tr("Configuration name cannot be blank") : null;
         }
      });
      if (dlg.open() != Window.OK)
         return;

      final String newName = dlg.getValue();
      new Job(i18n.tr("Updating agent configurations"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            AgentConfiguration configuration = session.getAgentConfiguration(h.getId());
            configuration.setName(newName);
            session.saveAgentConfig(configuration);

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
            return i18n.tr("Cannot update agent configuration");
         }
      }.start();
   }

   /**
    * Delete selected configurations
    */
   private void deleteConfig()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Confirm Delete"), i18n.tr("Selected configurations will be deleted. Are you sure?")))
         return;

      final List<Long> deleteList = new ArrayList<>();
      for(Object o : selection.toList())
         deleteList.add(((AgentConfigurationHandle)o).getId());

      new Job(i18n.tr("Deleting agent configurations"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(long id : deleteList)
               session.deleteAgentConfig(id);

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
            return i18n.tr("Cannot delete agent configuration");
         }
      }.start();
   }

   /**
    * Create new configuration
    */
   private void createNewConfig()
   {
      InputDialog dlg = new InputDialog(getWindow().getShell(), i18n.tr("Create New Configuration"), i18n.tr("Configuration name"), "", new IInputValidator() {
         @Override
         public String isValid(String newText)
         {
            return newText.isBlank() ? i18n.tr("Configuration name cannot be blank") : null;
         }
      });
      if (dlg.open() != Window.OK)
         return;

      final AgentConfiguration configuration = new AgentConfiguration();
      configuration.setName(dlg.getValue().trim());

      new Job(i18n.tr("Creating new agent configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            long configId = session.saveAgentConfig(configuration);
            final List<AgentConfigurationHandle> elements = session.getAgentConfigurations();
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  AgentConfigurationsManager.this.elements = elements;
                  viewer.setInput(elements);
                  for(int i = 0; i < elements.size(); i++)
                  {
                     AgentConfigurationHandle h = elements.get(i);
                     if (h.getId() == configId)
                     {
                        viewer.setSelection(new StructuredSelection(h));
                        editConfig();
                        break;
                     }
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create agent configurations");
         }
      }.start();
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionNew);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      final long[] currentSelection = new long[selection.size()];
      int i = 0;
      for(Object o : selection.toList())
         currentSelection[i++] = ((AgentConfigurationHandle)o).getId();

      new Job(i18n.tr("Reading list of agent configurations"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AgentConfigurationHandle> elements = session.getAgentConfigurations();
            runInUIThread(() -> {
               if (viewer.getControl().isDisposed())
                  return;

               AgentConfigurationsManager.this.elements = elements;
               viewer.setInput(elements);

               if (currentSelection.length > 0)
               {
                  List<AgentConfigurationHandle> selection = new ArrayList<>();
                  for(long id : currentSelection)
                  {
                     for(int i = 0; i < elements.size(); i++)
                     {
                        AgentConfigurationHandle h = elements.get(i);
                        if (h.getId() == id)
                           selection.add(h);
                     }
                  }
                  viewer.setSelection(new StructuredSelection(selection));
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get agent configurations");
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

   /**
    * Filter for configuration list
    */
   private static class ConfigurationFilter extends ViewerFilter implements AbstractViewerFilter
   {
      private String filterString = null;

      /**
       * @see org.netxms.nxmc.base.views.AbstractViewerFilter#setFilterString(java.lang.String)
       */
      @Override
      public void setFilterString(String filterString)
      {
         this.filterString = (filterString != null) ? filterString.toLowerCase() : null;
      }

      /**
       * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
       */
      @Override
      public boolean select(Viewer viewer, Object parentElement, Object element)
      {
         return (filterString == null) || filterString.isEmpty() || ((AgentConfigurationHandle)element).getName().toLowerCase().contains(filterString);
      }
   }
}
