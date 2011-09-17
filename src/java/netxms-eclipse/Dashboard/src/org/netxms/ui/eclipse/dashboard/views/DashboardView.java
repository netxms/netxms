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

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.dashboard.widgets.DashboardControl;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Dashboard view
 */
public class DashboardView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.dashboard.views.DashboardView";
	
	private NXCSession session;
	private Dashboard dashboard;
	private DashboardControl dbc;
	private Action actionEditMode;
	private Action actionAddAlarmBrowser;
	private Action actionAddLabel;
	private Action actionAddPieChart;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		session = (NXCSession)ConsoleSharedData.getSession();
		dashboard = (Dashboard)session.findObjectById(Long.parseLong(site.getSecondaryId()));
		if (dashboard == null)
			throw new PartInitException("Dashboard object is no longer exist or is not accessible");
		setPartName("Dashboard: " + dashboard.getObjectName());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		dbc = new DashboardControl(parent, SWT.NONE, dashboard, false);

		createActions();
		contributeToActionBars();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionEditMode = new Action("Edit mode", Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				dbc.setEditMode(!dbc.isEditMode());
				actionEditMode.setChecked(dbc.isEditMode());
			}
		};
		actionEditMode.setImageDescriptor(SharedIcons.EDIT);
		actionEditMode.setChecked(dbc.isEditMode());
		
		actionAddAlarmBrowser = new Action("Add &alarm browser") {
			@Override
			public void run()
			{
				dbc.addAlarmBrowser();
			}
		};
		
		actionAddLabel = new Action("Add &label") {
			@Override
			public void run()
			{
				dbc.addLabel();
			}
		};
		
		actionAddPieChart = new Action("Add &pie chart") {
			@Override
			public void run()
			{
				dbc.addPieChart();
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
		manager.add(actionEditMode);
		manager.add(new Separator());
		manager.add(actionAddAlarmBrowser);
		manager.add(actionAddLabel);
		manager.add(actionAddPieChart);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionEditMode);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		dbc.setFocus();
	}
}
