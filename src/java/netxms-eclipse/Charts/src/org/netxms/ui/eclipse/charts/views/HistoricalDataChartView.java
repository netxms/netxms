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
package org.netxms.ui.eclipse.charts.views;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.ArrayList;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.charts.api.HistoricalDataChart;
import org.netxms.ui.eclipse.charts.widgets.DataComparisonBirtChart;
import org.netxms.ui.eclipse.charts.widgets.GenericChart;
import org.netxms.ui.eclipse.charts.widgets.HistoricalDataBirtChart;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * View for historical data charts
 *
 */
public class HistoricalDataChartView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.charts.views.HistoricalDataChartView";
	
	private HistoricalDataChart chart;
	private GenericChart chartWidget;
	protected NXCSession session;
	private boolean updateInProgress = false;
	protected ArrayList<GraphItem> items = new ArrayList<GraphItem>(8);
	private Runnable refreshTimer;
	private boolean autoRefreshEnabled = true;
	private boolean useLogScale = false;
	private boolean showIn3D = false;
	private int autoRefreshInterval = 30000;	// 30 seconds

	private RefreshAction actionRefresh;
	private Action actionAutoRefresh;
	private Action actionShowIn3D;
	private Action actionUseLogScale;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		session = (NXCSession)ConsoleSharedData.getSession();

		// Extract information from view id
		//   first field is unique ID
		//   second is DCI list
		String id = site.getSecondaryId();
		String[] fields = id.split("&");
		if (!fields[0].equals(HistoryGraph.PREDEFINED_GRAPH_SUBID))
		{
			for(int i = 1; i < fields.length; i++)
			{
				String[] subfields = fields[i].split("\\@");
				if (subfields.length == 6)
				{
					try
					{
						items.add(new GraphItem(Long.parseLong(subfields[0], 10), // Node ID 
								Long.parseLong(subfields[1], 10), // DCI ID
								Integer.parseInt(subfields[2], 10), // source
								Integer.parseInt(subfields[3], 10), // data type
								URLDecoder.decode(subfields[4], "UTF-8"), // name
								URLDecoder.decode(subfields[5], "UTF-8"))); // description
					}
					catch(NumberFormatException e)
					{
						e.printStackTrace();
					}
					catch(UnsupportedEncodingException e)
					{
						e.printStackTrace();
					}
				}
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		chart = new HistoricalDataBirtChart(parent, SWT.NONE);
		chartWidget = (GenericChart)chart;
		
		for(GraphItem item : items)
			chart.addParameter(item);
		
		chart.initializationComplete();
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		updateChart();

		final Display display = getSite().getShell().getDisplay();
		refreshTimer = new Runnable()
		{
			@Override
			public void run()
			{
				if (chartWidget.isDisposed())
					return;

				updateChart();
				if (autoRefreshEnabled)
					display.timerExec(autoRefreshInterval, this);
			}
		};
		if (autoRefreshEnabled)
			getSite().getShell().getDisplay().timerExec(autoRefreshInterval, refreshTimer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		chartWidget.setFocus();
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

		// Create menu.
		Menu menu = menuMgr.createContextMenu(chartWidget);
		chartWidget.setMenu(menu);
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
				updateChart();
			}
		};
		
		actionAutoRefresh = new Action("Refresh &automatically") {
			@Override
			public void run()
			{
				autoRefreshEnabled = !autoRefreshEnabled;
				setChecked(autoRefreshEnabled);
				getSite().getShell().getDisplay().timerExec(autoRefreshEnabled ? autoRefreshInterval : -1, refreshTimer);
			}
		};
		actionAutoRefresh.setChecked(autoRefreshEnabled);
		
		actionUseLogScale = new Action("&Logarithmic scale") {
			@Override
			public void run()
			{
				useLogScale = !useLogScale;
				setChecked(useLogScale);
				chart.setLogScaleEnabled(useLogScale);
			}
		};
		actionUseLogScale.setChecked(useLogScale);
		
		actionShowIn3D = new Action("Show in &3D") {
			@Override
			public void run()
			{
				showIn3D = !showIn3D;
				setChecked(showIn3D);
				chart.set3DModeEnabled(showIn3D);
			}
		};
		actionShowIn3D.setChecked(showIn3D);
		actionShowIn3D.setImageDescriptor(Activator.getImageDescriptor("icons/view3d.png"));
	}
	
	/**
	 * Fill action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * @param manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionShowIn3D);
		manager.add(actionUseLogScale);
		manager.add(actionAutoRefresh);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill context menu
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionShowIn3D);
		manager.add(actionUseLogScale);
		manager.add(actionAutoRefresh);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionShowIn3D);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Get DCI data from server
	 */
	private void updateChart()
	{
		if (updateInProgress)
			return;
		
		updateInProgress = true;
		ConsoleJob job = new ConsoleJob("Get DCI values for chart", this, Activator.PLUGIN_ID, Activator.PLUGIN_ID)
		{
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get DCI data";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final DciData[] values = new DciData[items.size()];
				for(int i = 0; i < items.size(); i++)
				{
					GraphItem item = items.get(i);
					DciData data = session.getCollectedData(item.getNodeId(), item.getDciId(), null, null, 100);
				}

				new UIJob("Update chart")
				{
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						setChartData(values);
						updateInProgress = false;
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		};
		job.setUser(false);
		job.schedule();
	}
	
	/**
	 * Set new data for chart items
	 * 
	 * @param values new values
	 */
	private void setChartData(DciData[] values)
	{
		for(int i = 0; i < values.length; i++)
			chart.updateParameter(i, values[i], false);
		chart.refresh();
	}
}
