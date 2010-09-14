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
import org.eclipse.swt.widgets.Control;
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
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.widgets.BarChart;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.RefreshAction;
import org.swtchart.Chart;
import org.swtchart.IBarSeries;

/**
 * Bar chart view for DCIs
 *
 */
public class LastValuesChart extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.charts.views.LastValuesChart";
	
	private NXCSession session;
	private ArrayList<GraphItem> items = new ArrayList<GraphItem>(8);
	private Chart chart;				// Current chart
	private BarChart barChart;
	private boolean updateInProgress = false;
	private Runnable refreshTimer;
	private boolean autoRefreshEnabled = true;
	private boolean useLogScale = false;
	private int autoRefreshInterval = 30000;	// 30 seconds

	private RefreshAction actionRefresh;
	private Action actionAutoRefresh;
	private Action actionLogScale;
	private Action actionShowVertical;
	private Action actionShowHorizontal;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		final Display display = site.getShell().getDisplay();
		refreshTimer = new Runnable() {
			@Override
			public void run()
			{
				if (chart.isDisposed())
					return;
				
				updateChart();
				display.timerExec(autoRefreshInterval, this);
			}
		};
		
		session = NXMCSharedData.getInstance().getSession();
		
		// Extract DCI ids from view id
		// (first field will be unique view id, so we skip it)
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
						items.add(new GraphItem(
								Long.parseLong(subfields[0], 10),	// Node ID 
								Long.parseLong(subfields[1], 10),	// DCI ID
								Integer.parseInt(subfields[2], 10),	// source
								Integer.parseInt(subfields[3], 10),	// data type
								URLDecoder.decode(subfields[4], "UTF-8"),   // name
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
		barChart = new BarChart(parent, SWT.NONE);
		barChart.setCategories(new String[] { "" });
		barChart.getAxisSet().getYAxis(0).enableLogScale(useLogScale);
		chart = barChart;

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		getDataFromServer();
		if (autoRefreshEnabled)
			getSite().getShell().getDisplay().timerExec(autoRefreshInterval, refreshTimer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		chart.setFocus();
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
		Menu menu = menuMgr.createContextMenu(chart);
		chart.setMenu(menu);
		for(Control ch : chart.getChildren())
		{
			ch.setMenu(menu);
		}
	}

	/**
	 * Fill context menu
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionShowVertical);
		manager.add(actionShowHorizontal);
		manager.add(new Separator());
		manager.add(actionAutoRefresh);
		manager.add(actionLogScale);
		manager.add(new Separator());
		manager.add(actionRefresh);
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
		manager.add(actionShowVertical);
		manager.add(actionShowHorizontal);
		manager.add(new Separator());
		manager.add(actionAutoRefresh);
		manager.add(actionLogScale);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionShowVertical);
		manager.add(actionShowHorizontal);
		manager.add(new Separator());
		manager.add(actionRefresh);
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
		
		actionAutoRefresh = new Action() {
			@Override
			public void run()
			{
				autoRefreshEnabled = !autoRefreshEnabled;
				setChecked(autoRefreshEnabled);
				LastValuesChart.this.getSite().getShell().getDisplay().timerExec(autoRefreshEnabled ? autoRefreshInterval : -1, refreshTimer);
			}
		};
		actionAutoRefresh.setText("Refresh &automatically");
		actionAutoRefresh.setChecked(autoRefreshEnabled);
		
		actionLogScale = new Action() {
			@Override
			public void run()
			{
				useLogScale = !useLogScale;
				setChecked(useLogScale);
				chart.getAxisSet().getYAxis(0).enableLogScale(useLogScale);
				chart.redraw();
			}
		};
		actionLogScale.setText("&Logarithmic scale");
		actionLogScale.setChecked(useLogScale);
		
		actionShowVertical = new Action() {
			@Override
			public void run()
			{
				setChecked(true);
				actionShowHorizontal.setChecked(false);
				barChart.setOrientation(SWT.HORIZONTAL);
				barChart.redraw();
			}
		};
		actionShowVertical.setText("Vertical");
		actionShowVertical.setImageDescriptor(Activator.getImageDescriptor("icons/bar_vertical.png"));
		actionShowVertical.setChecked(barChart.getOrientation() == SWT.HORIZONTAL);
		
		actionShowHorizontal = new Action() {
			@Override
			public void run()
			{
				barChart.setOrientation(SWT.VERTICAL);
				setChecked(true);
				actionShowVertical.setChecked(false);
				barChart.redraw();
			}
		};
		actionShowHorizontal.setText("Horizontal");
		actionShowHorizontal.setImageDescriptor(Activator.getImageDescriptor("icons/bar_horizontal.png"));
		actionShowHorizontal.setChecked(barChart.getOrientation() == SWT.VERTICAL);
	}

	/**
	 * Update chart
	 */
	private void updateChart()
	{
		if (updateInProgress)
			return;
		
		updateInProgress = true;
		getDataFromServer();
	}

	/**
	 * Get DCI data from server
	 */
	private void getDataFromServer()
	{
		ConsoleJob job = new ConsoleJob("Get DCI values for chart", this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get DCI data";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final double[] values = new double[items.size()];
				for(int i = 0; i < items.size(); i++)
				{
					GraphItem item = items.get(i);
					DciData data = session.getCollectedData(item.getNodeId(), item.getDciId(), null, null, 1);
					DciDataRow value = data.getLastValue();
					values[i] = (value != null) ? value.getValueAsDouble() : 0.0;
				}
				
				new UIJob("Update chart") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						setChartData(values);
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		};
		job.setUser(false);
		job.schedule();
	}
	
	/**
	 * Set chart data
	 * 
	 * @param data Retrieved DCI data
	 */
	private void setChartData(final double[] data)
	{
		for(int i = 0; i < data.length; i++)
			addItemToChart(i, items.get(i), data[i], i > 0);
		
		chart.getAxisSet().getYAxis(0).adjustRange();
		chart.redraw();
	}
	
	/**
	 * Add single DCI to chart
	 * 
	 * @param data DCI data
	 */
	private void addItemToChart(int index, GraphItem item, double data, boolean padding)
	{
		IBarSeries series = barChart.addBarSeries(index, item.getDescription(), data);
		series.setBarMargin(25);
	}
}
