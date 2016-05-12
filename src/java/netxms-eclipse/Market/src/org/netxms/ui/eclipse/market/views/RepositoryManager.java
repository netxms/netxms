/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.market.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
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
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.market.Repository;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.market.Activator;
import org.netxms.ui.eclipse.market.dialogs.RepositoryPropertiesDlg;
import org.netxms.ui.eclipse.market.objects.RepositoryRuntimeInfo;
import org.netxms.ui.eclipse.market.views.helpers.RepositoryContentProvider;
import org.netxms.ui.eclipse.market.views.helpers.RepositoryFilter;
import org.netxms.ui.eclipse.market.views.helpers.RepositoryLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Repository manager view
 */
public class RepositoryManager extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.market.views.RepositoryManager";
   
   private static final String TABLE_CONFIG_PREFIX = "RepositoryManager";
   
   private NXCSession session = ConsoleSharedData.getSession();
   private boolean filterEnabled = false;
   private RepositoryFilter filter;
   private Composite content;
   private FilterText filterText;
   private SortableTreeViewer viewer;
   private Action actionRefresh;
   private Action actionShowFilter;
   private Action actionAddRepository;
   private Action actionDelete;
   
   /* (non-Javadoc)
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
         }
      });
      
      final String[] columnNames = { "Name", "Version", "GUID" };
      final int[] columnWidths = { 300, 120, 150 };         
      viewer = new SortableTreeViewer(content, columnNames, columnWidths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
      
      WidgetHelper.restoreTreeViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
      viewer.setContentProvider(new RepositoryContentProvider(this));
      viewer.setLabelProvider(new RepositoryLabelProvider());
      //viewer.setComparator(new AgentFileComparator());
      filter = new RepositoryFilter();
      viewer.addFilter(filter);
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            if (selection != null)
            {
               actionDelete.setEnabled(selection.size() > 0);
            }
         }
      });
      viewer.getTree().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTreeViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
         }
      });

      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getTree().setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);

      createActions();
      contributeToActionBars();
      createPopupMenu();
      activateContext();

      filterText.setCloseAction(actionShowFilter);

      // Set initial focus to filter input line
      if (filterEnabled)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
      
      refreshRepositoryList();
   }

   /* (non-Javadoc)
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
         contextService.activateContext("org.netxms.ui.eclipse.market.context.RepositoryManager"); //$NON-NLS-1$
      }
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);

      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refreshRepositoryList();
         }
      };
      
      actionShowFilter = new Action("Show &filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(!filterEnabled);
            actionShowFilter.setChecked(filterEnabled);
         }
      };
      actionShowFilter.setChecked(filterEnabled);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.market.commands.showFilter"); //$NON-NLS-1$
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
      
      actionAddRepository = new Action("&Add repository...", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            addRepository();
         }
      };
      
      actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteRepository();
         }
      };
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
      manager.add(actionAddRepository);
      manager.add(new Separator());
      manager.add(actionShowFilter);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local tool bar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionAddRepository);
      manager.add(new Separator());
      manager.add(actionRefresh);
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

      // Register menu for extension.
      getSite().registerContextMenu(menuMgr, viewer);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager mgr)
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      
      if ((selection.size() == 1) && (selection.getFirstElement() instanceof RepositoryRuntimeInfo))
      {
         mgr.add(actionDelete);
      }
   }
   
   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      filterText.setVisible(filterEnabled);
      FormData fd = (FormData)viewer.getTree().getLayoutData();
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
   
   /**
    * Refresh repository list
    */
   private void refreshRepositoryList()
   {
      new ConsoleJob("Get list of configured repositories", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<Repository> repositories = session.getRepositories();
            final List<RepositoryRuntimeInfo> repositoryInfo = new ArrayList<RepositoryRuntimeInfo>(repositories.size());
            for(Repository r : repositories)
               repositoryInfo.add(new RepositoryRuntimeInfo(r));
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(repositoryInfo);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot get list of configured repositories";
         }
      }.start();
   }
   
   /**
    * Add repository
    */
   private void addRepository()
   {
      RepositoryPropertiesDlg dlg = new RepositoryPropertiesDlg(getSite().getShell(), null);
      if (dlg.open() != Window.OK)
         return;
      
      final Repository repository = new Repository(dlg.getUrl(), dlg.getToken(), dlg.getDescription());
      new ConsoleJob("Add repository", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.addRepository(repository);
            runInUIThread(new Runnable() {
               @SuppressWarnings("unchecked")
               @Override
               public void run()
               {
                  ArrayList<RepositoryRuntimeInfo> repositories = (ArrayList<RepositoryRuntimeInfo>)viewer.getInput();
                  repositories.add(new RepositoryRuntimeInfo(repository));
                  viewer.refresh();
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot add repository";
         }
      }.start();
   }
   
   /**
    * Delete repository
    */
   private void deleteRepository()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if ((selection.size() != 1) || !(selection.getFirstElement() instanceof RepositoryRuntimeInfo))
         return;
      
      final RepositoryRuntimeInfo repository = (RepositoryRuntimeInfo)selection.getFirstElement();
      if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Confirm Delete", 
            String.format("Repository %s will be deleted. Are you sure?", repository.getName())))
         return;
      
      new ConsoleJob("Delete repository", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.deleteRepository(repository.getRepositoryId());
            runInUIThread(new Runnable() {
               @SuppressWarnings("unchecked")
               @Override
               public void run()
               {
                  ArrayList<RepositoryRuntimeInfo> repositories = (ArrayList<RepositoryRuntimeInfo>)viewer.getInput();
                  repositories.remove(repository);
                  viewer.refresh();
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Cannot delete repository";
         }
      }.start();
   }
}
