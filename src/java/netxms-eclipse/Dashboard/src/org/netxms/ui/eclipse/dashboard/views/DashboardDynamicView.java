/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.views;

import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISelectionListener;
import org.eclipse.ui.ISelectionService;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.DashboardControl;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Dynamic dashboard view - change dashboard when selection in dashboard
 * navigator changes
 */
public class DashboardDynamicView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.dashboard.views.DashboardDynamicView"; //$NON-NLS-1$

	private Dashboard dashboard = null;
	private DashboardControl dbc = null;
	private ISelectionService selectionService;
	private ISelectionListener selectionListener;
	private Composite parentComposite;
	private RefreshAction actionRefresh;

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets
	 * .Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		parentComposite = parent;
		if (dashboard != null)
			dbc = new DashboardControl(parent, SWT.NONE, dashboard, this, false);

		createActions();
		contributeToActionBars();

		selectionService = getSite().getWorkbenchWindow().getSelectionService();
		selectionListener = new ISelectionListener() {
			@Override
			public void selectionChanged(IWorkbenchPart part, ISelection selection)
			{
				if ((part instanceof DashboardNavigator) && (selection instanceof IStructuredSelection) && !selection.isEmpty())
				{
					Object object = ((IStructuredSelection)selection).getFirstElement();
					if (object instanceof Dashboard)
					{
						setObject((Dashboard)object);
					}
				}
			}
		};
		selectionService.addSelectionListener(selectionListener);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				rebuildDashboard();
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
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
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

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		if (dbc != null)
		{
			dbc.setFocus();
		}
	}

	/**
	 * @param object
	 */
	private void setObject(Dashboard object)
	{
		if (dbc != null)
			dbc.dispose();
		dashboard = object;
		dbc = new DashboardControl(parentComposite, SWT.NONE, dashboard, this, false);
		parentComposite.layout();
		setPartName(Messages.DashboardDynamicView_PartNamePrefix + dashboard.getObjectName());
	}
	
	/**
	 * Rebuild current dashboard
	 */
	private void rebuildDashboard()
	{
		if (dashboard == null)
			return;
		
		if (dbc != null)
			dbc.dispose();
		dashboard = (Dashboard)((NXCSession)ConsoleSharedData.getSession()).findObjectById(dashboard.getObjectId(), Dashboard.class);
		if (dashboard != null)
		{
			dbc = new DashboardControl(parentComposite, SWT.NONE, dashboard, this, false);
			parentComposite.layout();
			setPartName(Messages.DashboardDynamicView_PartNamePrefix + dashboard.getObjectName());
		}
		else
		{
			dbc = null;
		}
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((selectionService != null) && (selectionListener != null))
			selectionService.removeSelectionListener(selectionListener);
		super.dispose();
	}
}
