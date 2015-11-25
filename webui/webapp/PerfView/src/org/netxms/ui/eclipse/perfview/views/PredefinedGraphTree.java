/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IElementComparer;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.PredefinedChartConfig;
import org.netxms.ui.eclipse.perfview.views.helpers.GraphFolder;
import org.netxms.ui.eclipse.perfview.views.helpers.GraphTreeContentProvider;
import org.netxms.ui.eclipse.perfview.views.helpers.GraphTreeFilter;
import org.netxms.ui.eclipse.perfview.views.helpers.GraphTreeLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.FilterText;

/**
 * Navigation view for predefined graphs
 */
@SuppressWarnings("restriction")
public class PredefinedGraphTree extends ViewPart implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.perfview.views.PredefinedGraphTree"; //$NON-NLS-1$
	
	private TreeViewer viewer;
   private FilterText filterText;
   private boolean filterEnabled = true;
	private NXCSession session;
	private RefreshAction actionRefresh;
	private Action actionOpen; 
	private Action actionProperties; 
	private Action actionDelete;
	private Action actionShowFilter;
   private GraphTreeFilter filter; 

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		parent.setLayout(new FormLayout());
		
      // Create filter area
      filterText = new FilterText(parent, SWT.NONE);
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
		
		viewer = new TreeViewer(parent, SWT.NONE);
		viewer.setUseHashlookup(true);
		viewer.setContentProvider(new GraphTreeContentProvider());
		viewer.setLabelProvider(new GraphTreeLabelProvider());
		viewer.setComparer(new IElementComparer() {
         @Override
         public int hashCode(Object element)
         {
            if((element instanceof GraphSettings))
            {
               return (int)((GraphSettings)element).getId();
            }
            if((element instanceof GraphFolder))
            {
               return ((GraphFolder)element).getName().hashCode();
            }
            return element.hashCode();
         }
         
         @Override
         public boolean equals(Object a, Object b)
         {
            if ((a instanceof GraphSettings) && (b instanceof GraphSettings))
               return ((GraphSettings)a).getId() == ((GraphSettings)b).getId();
            if ((a instanceof GraphFolder) && (b instanceof GraphFolder))
               return ((GraphFolder)a).getName().equals(((GraphFolder)b).getName());
            return a.equals(b);
         }
      });		
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				actionOpen.run();
			}
		});
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@SuppressWarnings("rawtypes")
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				Iterator it = selection.iterator();
				boolean enabled = false;
				while(it.hasNext())
				{
					if (it.next() instanceof GraphSettings)
					{
						enabled = true;
						break;
					}
				}
				actionOpen.setEnabled(enabled);
				actionDelete.setEnabled(enabled);
				actionProperties.setEnabled(enabled);
			}
		});
		filter = new GraphTreeFilter();
		viewer.addFilter(filter);

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
		
      activateContext();
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		reloadGraphList();
      session.addListener(this);

      // Set initial focus to filter input line
      if (filterEnabled)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
	   if (filterEnabled)
	      filterText.setFocus();
	   else
	      viewer.getTree().setFocus();
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
				reloadGraphList();
			}
		};
		
		actionDelete = new Action(Messages.get().PredefinedGraphTree_Delete, SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deletePredefinedGraph();
			}
		};
		
		actionOpen = new Action() {
			@SuppressWarnings("rawtypes")
			@Override
			public void run()
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				Iterator it = selection.iterator();
				while(it.hasNext())
				{
					Object o = it.next();
					if (o instanceof GraphSettings)
					{
						showPredefinedGraph((GraphSettings)o);
					}
				}
			}
		};
		actionOpen.setText(Messages.get().PredefinedGraphTree_Open);

		actionProperties = new Action(Messages.get().PredefinedGraphTree_Properties) {
			@Override
			public void run()
			{
				editPredefinedGraph();
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
      actionShowFilter.setId("org.netxms.ui.eclipse.perfview.actions.showFilter"); //$NON-NLS-1$
      actionShowFilter.setChecked(filterEnabled);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.perfview.commands.show_graph_filter"); //$NON-NLS-1$
      final ActionHandler showFilterHandler = new ActionHandler(actionShowFilter);
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), showFilterHandler);
	}
	
   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.perfview.context.PredefinedGraphTree"); //$NON-NLS-1$
      }
   }
	
	/**
	 * Create pop-up menu for user list
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
	   if (selection.getFirstElement() instanceof GraphFolder)
	      return;
	   
		mgr.add(actionOpen);
		mgr.add(actionDelete);
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(actionProperties);
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
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(actionShowFilter);
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
		manager.add(actionRefresh);
	}
	
	/**
	 * Reload graph list from server
	 */
	private void reloadGraphList()
	{
		new ConsoleJob(Messages.get().PredefinedGraphTree_LoadJobName, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().PredefinedGraphTree_LoadJobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<GraphSettings> list = session.getPredefinedGraphs(false);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(list);
					}
				});
			}
		}.start();
	}
	
	/**
	 * Show predefined graph view
	 * 
	 * @param gs graph settings
	 */
	private void showPredefinedGraph(GraphSettings gs)
	{
		String encodedName;
		try
		{
			encodedName = URLEncoder.encode(gs.getName(), "UTF-8"); //$NON-NLS-1$
		}
		catch(UnsupportedEncodingException e1)
		{
			encodedName = "___ERROR___"; //$NON-NLS-1$
		}
		String id = HistoricalGraphView.PREDEFINED_GRAPH_SUBID + "&" + encodedName; //$NON-NLS-1$
		try
		{
			HistoricalGraphView g = (HistoricalGraphView)PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().showView(HistoricalGraphView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
			if (g != null)
				g.initPredefinedGraph(gs);
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), Messages.get().PredefinedGraphTree_Error, String.format(Messages.get().PredefinedGraphTree_ErrorOpeningView, e.getLocalizedMessage()));
		}
	}
	
	/**
	 * Edit predefined graph
	 */
	private void editPredefinedGraph()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		GraphSettings settings = (GraphSettings)selection.getFirstElement();
		PredefinedChartConfig config;
		try
		{
			config = PredefinedChartConfig.createFromServerConfig(settings);
		}
		catch(Exception e)
		{
			config = new PredefinedChartConfig();
		}
		PropertyDialog dlg = PropertyDialog.createDialogOn(getSite().getShell(), null, config);
		if (dlg != null)
		{
			if (dlg.open() == Window.OK)
			{
				try
				{
					final GraphSettings s = config.createServerSettings();
					new ConsoleJob(Messages.get().PredefinedGraphTree_UpdateJobName, null, Activator.PLUGIN_ID, null) {
						@Override
						protected void runInternal(IProgressMonitor monitor) throws Exception
						{
							session.saveGraph(s, true);
						}
						
						@Override
						protected String getErrorMessage()
						{
							return Messages.get().PredefinedGraphTree_UpdateJobError;
						}
					}.start();
				}
				catch(Exception e)
				{
					MessageDialogHelper.openError(getSite().getShell(), "Internal Error", String.format("Unexpected exception: %s", e.getLocalizedMessage())); //$NON-NLS-1$ //$NON-NLS-2$
				}
			}
			settings.setName(config.getName());
			settings.getAccessList().clear();
			settings.getAccessList().addAll(config.getAccessList());
			try
			{
				settings.setConfig(config.createXml());
			}
			catch(Exception e)
			{
				MessageDialogHelper.openError(getSite().getShell(), "Internal Error", String.format("Unexpected exception: %s", e.getLocalizedMessage())); //$NON-NLS-1$ //$NON-NLS-2$
			}
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
               @SuppressWarnings("unchecked")
               @Override
               public void run()
               {
                  List<GraphSettings> list = (List<GraphSettings>)viewer.getInput();    
                  for(int i = 0; i < list.size(); i++)
                     if(list.get(i).getId() == n.getSubCode())
                     {
                        Object o = list.get(i);
                        list.remove(o);
                        viewer.refresh();
                        break;
                     }
               }
            });
            break;
         case SessionNotification.PREDEFINED_GRAPHS_CHANGED:            
            viewer.getControl().getDisplay().asyncExec(new Runnable() {
               @SuppressWarnings("unchecked")
               @Override
               public void run()
               {
                  if(!(n.getObject() instanceof GraphSettings))
                  {
                     return;
                  }                       
                  
                  final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();  
                  
                  final List<GraphSettings> list = (List<GraphSettings>)viewer.getInput();       
                  boolean objectUpdated = false;
                  for(int i = 0; i < list.size(); i++)
                  {
                     if(list.get(i).getId() == n.getSubCode())
                     {
                        list.set(i, (GraphSettings)n.getObject());
                        objectUpdated = true;
                        break;
                     }
                  }
                  
                  if(!objectUpdated)
                  {
                     list.add((GraphSettings)n.getObject());
                  }
                  viewer.refresh();
                  
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
      filterText.getParent().layout(true, true);
      if (enable)
         filterText.setFocus();
      else
         setFilter(""); //$NON-NLS-1$
   }

   /**
    * Set filter text
    * 
    * @param text New filter text
    */
   private void setFilter(final String text)
   {
      filterText.setText(text);
      onFilterModify();
   }

   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
      
      GraphSettings s = filter.getLastMatch();
      if (s != null)
      {
         viewer.expandToLevel(s, 1);
         viewer.setSelection(new StructuredSelection(s), true);
      }
   }
}
