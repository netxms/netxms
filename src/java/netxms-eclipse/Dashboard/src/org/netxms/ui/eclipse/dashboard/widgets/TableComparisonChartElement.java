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
package org.netxms.ui.eclipse.dashboard.widgets;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.DataChart;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TableComparisonChartConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Base class for data comparison charts based on table DCI - like bar chart, pie chart, etc.
 */
public abstract class TableComparisonChartElement extends ElementWidget
{
	protected DataComparisonChart chart;
	protected NXCSession session;
	protected int refreshInterval = 30000;
	protected TableComparisonChartConfig config;
	
	private Runnable refreshTimer;
	private boolean updateInProgress = false;
	private Map<String, Integer> instanceMap = new HashMap<String, Integer>(DataChart.MAX_CHART_ITEMS);
	private boolean chartInitialized = false;

	/**
	 * @param parent
	 * @param data
	 */
	public TableComparisonChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
		session = (NXCSession)ConsoleSharedData.getSession();

		setLayout(new FillLayout());	
	}
	
	/**
	 * Start refresh timer
	 */
	protected void startRefreshTimer()
	{
		if ((config == null) || (config.getDataColumn() == null))
			return;	// Invalid configuration
		
		final Display display = getDisplay();
		refreshTimer = new Runnable() {
			@Override
			public void run()
			{
				if (TableComparisonChartElement.this.isDisposed())
					return;
				
				refreshData();
				display.timerExec(refreshInterval, this);
			}
		};
		display.timerExec(refreshInterval, refreshTimer);
		refreshData();
	}

	/**
	 * Refresh graph's data
	 */
	protected void refreshData()
	{
		if (updateInProgress)
			return;
		
		updateInProgress = true;
		
		ConsoleJob job = new ConsoleJob(Messages.TableComparisonChartElement_JobTitle, viewPart, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Table data = session.getTableLastValues(config.getNodeId(), config.getDciId());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (!((Widget)chart).isDisposed())
							updateChart(data);
						updateInProgress = false;
					}
				});
			}
	
			@Override
			protected String getErrorMessage()
			{
				return Messages.TableComparisonChartElement_JobError;
			}
	
			@Override
			protected void jobFailureHandler()
			{
				updateInProgress = false;
				super.jobFailureHandler();
			}
		};
		job.setUser(false);
		job.start();
	}
	
	/**
	 * Update chart with new data
	 * 
	 * @param data
	 */
	private void updateChart(Table data)
	{
		String instanceColumn = (config.getInstanceColumn() != null) ? config.getInstanceColumn() : ""; // FIXME //$NON-NLS-1$
		if (instanceColumn == null)
			return;
		
		int icIndex = data.getColumnIndex(instanceColumn);
		int dcIndex = data.getColumnIndex(config.getDataColumn());
		if ((icIndex == -1) || (dcIndex == -1))
			return;	// at least one column is missing

		boolean rebuild = false;
		for(int i = 0; i < data.getRowCount(); i++)
		{
			String instance = data.getCell(i, icIndex);
			if (instance == null)
				continue;

			double value;
			try
			{
				value = Double.parseDouble(data.getCell(i, dcIndex));
			}
			catch(NumberFormatException e)
			{
				value = 0.0;
			}
			
			Integer index = instanceMap.get(instance);
			if (index == null)
			{
				if ((instanceMap.size() >= DataChart.MAX_CHART_ITEMS) ||
				    ((value == 0) && config.isIgnoreZeroValues()))
					continue;
				index = chart.addParameter(new GraphItem(config.getNodeId(), config.getDciId(), 0, 0, Long.toString(config.getDciId()), instance), 0.0);
				instanceMap.put(instance, index);
				rebuild = true;
			}

			chart.updateParameter(index, value, false);
		}

		if (!chartInitialized)
		{
			chart.initializationComplete();
			chartInitialized = true;
		}
		else
		{
			if (rebuild)
				chart.rebuild();
			else
				chart.refresh();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		Point size = super.computeSize(wHint, hHint, changed);
		if ((hHint == SWT.DEFAULT) && (size.y < 250))
			size.y = 250;
		return size;
	}
}
