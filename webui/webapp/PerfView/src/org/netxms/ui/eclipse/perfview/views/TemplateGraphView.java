/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.dialogs.SaveGraphDlg;
import org.netxms.ui.eclipse.perfview.views.helpers.TemplateGraphLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Template Graph Configuration
 */
@SuppressWarnings("restriction")
public class TemplateGraphView extends ViewPart implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.perfview.views.TemplateGraphView"; //$NON-NLS-1$
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_DCI_NAME = 1;
   public static final int COLUMN_DCI_DESCRIPTION = 2;
	
	private NXCSession session;
	private SortableTableViewer viewer;
	private Action actionEdit;
	private Action actionDelete;
	private Action actionAdd;
	private Action actionRefresh;
   private boolean updateInProgress = false;
   List<GraphSettings> graphList;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		session = (NXCSession)ConsoleSharedData.getSession();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { "Graph name", "DCI names", "DCI descriptions" };
		final int[] widths = { 150, 400, 400 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new TemplateGraphLabelProvider());
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();
         }
      });

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		refreshData();
      session.addListener(this);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refreshData();
			}
		};
		
		actionEdit = new Action("Edit graph") {
			@Override
			public void run()
			{
			   editTemplateGraph();
			}
		};
		
		actionAdd = new Action("Create new template graph", SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            saveGraph("", null, false);
         }
      };
      
      actionDelete = new Action("Delete template graph") {
         @Override
         public void run()
         {
            deletePredefinedGraph();
         }
      };
	}
	
   /**
    * Save this graph as predefined
    */
   private void saveGraph(String graphName, String errorMessage, final boolean canBeOverwritten)
   {
      SaveGraphDlg dlg = new SaveGraphDlg(getSite().getShell(), graphName, errorMessage, canBeOverwritten);
      int result = dlg.open();
      if (result == Window.CANCEL)
         return;

      final GraphSettings gs = new GraphSettings(0, session.getUserId(), 0, new ArrayList<AccessListElement>(0));
      gs.setName(dlg.getName());
      gs.setFlags(GraphSettings.GRAPH_FLAG_TEMPLATE);

      if (result == SaveGraphDlg.OVERRIDE)
      {
         new ConsoleJob(Messages.get().HistoricalGraphView_SaveSettings, this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.saveGraph(gs, canBeOverwritten);
            }

            @Override
            protected String getErrorMessage()
            {
               return Messages.get().HistoricalGraphView_SaveSettingsError;
            }
         }.start();
      }
      else
      {
         new ConsoleJob(Messages.get().HistoricalGraphView_SaveSettings, this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  session.saveGraph(gs, canBeOverwritten);
               }
               catch(NXCException e)
               {
                  if (e.getErrorCode() == RCC.OBJECT_ALREADY_EXISTS)
                  {
                     runInUIThread(new Runnable() {

                        @Override
                        public void run()
                        {
                           saveGraph(gs.getName(), Messages.get().HistoricalGraphView_NameAlreadyExist, true);
                        }

                     });
                  }
                  else
                  {
                     if (e.getErrorCode() == RCC.ACCESS_DENIED)
                     {
                        runInUIThread(new Runnable() {

                           @Override
                           public void run()
                           {
                              saveGraph(gs.getName(), Messages.get().HistoricalGraphView_NameAlreadyExistNoOverwrite, false);
                           }

                        });
                     }
                     else
                     {
                        throw e;
                     }
                  }
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return Messages.get().HistoricalGraphView_SaveError;
            }
         }.start();
      }
   }
	
   /* (non-Javadoc)
    * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
    */
   @Override
   public void notificationHandler(final SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.PREDEFINED_GRAPHS_DELETED:
            viewer.getControl().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               { 
                  for(int i = 0; i < graphList.size(); i++)
                     if(graphList.get(i).getId() == n.getSubCode())
                     {
                        Object o = graphList.get(i);
                        graphList.remove(o);
                        viewer.setInput(graphList);
                        break;
                     }
               }
            });
            break;
         case SessionNotification.PREDEFINED_GRAPHS_CHANGED:            
            viewer.getControl().getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  if(!(n.getObject() instanceof GraphSettings))
                  {
                     return;
                  }                       
                  
                  final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();  
                  
                  boolean objectUpdated = false;
                  for(int i = 0; i < graphList.size(); i++)
                  {
                     if(graphList.get(i).getId() == n.getSubCode())
                     {
                        graphList.set(i, (GraphSettings)n.getObject());
                        objectUpdated = true;
                        break;
                     }
                  }
                  
                  if(!objectUpdated)
                  {
                     if(((GraphSettings)n.getObject()).isTemplate())
                        graphList.add((GraphSettings)n.getObject());
                  }
                  viewer.setInput(graphList);
                  
                  if (selection.size() == 1)
                  {
                     if(selection.getFirstElement() instanceof GraphSettings)
                     {
                        GraphSettings element = (GraphSettings)selection.getFirstElement();
                        if(element.getId() == n.getSubCode())
                              viewer.setSelection(new StructuredSelection((GraphSettings)n.getObject()), true);
                     }
                  }
               }
            });
            break;
      }
   }

   /**
    * Edit template graph
    */
   private void editTemplateGraph()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      GraphSettings settings = (GraphSettings)selection.getFirstElement();
      PropertyDialog dlg = PropertyDialog.createDialogOn(getSite().getShell(), null, settings);
      final GraphSettings newSettings = settings;
      if (dlg != null)
      {
         if (dlg.open() == Window.OK)
         {
            try
            {
               new ConsoleJob("Update template graph", null, Activator.PLUGIN_ID, null) {
                  @Override
                  protected void runInternal(IProgressMonitor monitor) throws Exception
                  {
                     session.saveGraph(newSettings, true);
                  }
                  
                  @Override
                  protected String getErrorMessage()
                  {
                     return "Cannot update predefined graph";
                  }
               }.start();
            }
            catch(Exception e)
            {
               MessageDialogHelper.openError(getSite().getShell(), "Internal Error", String.format("Unexpected exception: %s", e.getLocalizedMessage())); //$NON-NLS-1$ //$NON-NLS-2$
            }
         }
         settings.setName(newSettings.getName());
         settings.getAccessList().clear();
         settings.getAccessList().addAll(newSettings.getAccessList());
         viewer.update(settings, null);
      }
   }
   
   /**
    * Delete predefined graph(s)
    */
   private void deletePredefinedGraph()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() == 0)
         return;
      
      if (!MessageDialogHelper.openQuestion(getSite().getShell(), Messages.get().PredefinedGraphTree_DeletePromptTitle, Messages.get().PredefinedGraphTree_DeletePromptText))
         return;
      
      for(final Object o : selection.toList())
      {
         if (!(o instanceof GraphSettings))
            continue;
         
         new ConsoleJob(String.format(Messages.get().PredefinedGraphTree_DeleteJobName, ((GraphSettings)o).getShortName()), null, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.deletePredefinedGraph(((GraphSettings)o).getId());
            }
            
            @Override
            protected String getErrorMessage()
            {
               return Messages.get().PredefinedGraphTree_DeleteJobError;
            }
         }.start();
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
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionAdd);
		manager.add(actionEdit);
      manager.add(actionDelete);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
      manager.add(actionAdd);
		manager.add(actionRefresh);
	}

	/**
	 * Create pop-up menu
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
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
      manager.add(actionAdd);
      manager.add(actionEdit);
      manager.add(actionDelete);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
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
	 * Refresh data
	 */
	private void refreshData()
	{
		if (updateInProgress )
			return;
		updateInProgress = true;
		
		new ConsoleJob(Messages.get().HistoricalDataView_RefreshJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<GraphSettings> settings = session.getPredefinedGraphs(true);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
					   graphList = settings;
						viewer.setInput(graphList.toArray());
						updateInProgress = false;
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
			   return String.format("Error getting ");
			}
		}.start();
	}
}
