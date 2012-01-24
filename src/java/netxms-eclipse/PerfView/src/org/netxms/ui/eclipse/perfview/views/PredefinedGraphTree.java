/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.views.helpers.GraphTreeContentProvider;
import org.netxms.ui.eclipse.perfview.views.helpers.GraphTreeLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Navigation view for predefined graphs
 *
 */
public class PredefinedGraphTree extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.perfview.views.PredefinedGraphTree";
	
	private TreeViewer viewer;
	private NXCSession session;
	private RefreshAction actionRefresh;
	private Action actionOpen; 

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		parent.setLayout(new FillLayout());
		
		viewer = new TreeViewer(parent, SWT.NONE);
		viewer.setContentProvider(new GraphTreeContentProvider());
		viewer.setLabelProvider(new GraphTreeLabelProvider());
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
			}
		});

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		reloadGraphList();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTree().setFocus();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				reloadGraphList();
			}
		};
		
		actionOpen = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
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
		actionOpen.setText("&Open");
	}
	
	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
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
		mgr.add(actionOpen);
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
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
		manager.add(actionOpen);
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
		new ConsoleJob("Load list of predefined graphs", this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get list of predefined graphs";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<GraphSettings> list = session.getPredefinedGraphs();
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
			encodedName = URLEncoder.encode(gs.getName(), "UTF-8");
		}
		catch(UnsupportedEncodingException e1)
		{
			encodedName = "___ERROR___";
		}
		String id = HistoricalDataView.PREDEFINED_GRAPH_SUBID + "&" + encodedName;
		try
		{
			HistoricalDataView g = (HistoricalDataView)PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().showView(HistoricalDataView.ID, id, IWorkbenchPage.VIEW_ACTIVATE);
			if (g != null)
				g.initPredefinedGraph(gs);
		}
		catch(PartInitException e)
		{
			MessageDialog.openError(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), "Error", "Error opening graph view: " + e.getMessage());
		}
	}
}
