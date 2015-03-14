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
package org.netxms.ui.eclipse.perfview.objecttabs.internal;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.ui.eclipse.charts.api.ChartFactory;
import org.netxms.ui.eclipse.charts.api.HistoricalDataChart;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.PerfTabGraphSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ViewRefreshController;
import org.netxms.ui.eclipse.tools.VisibilityValidator;
import org.netxms.ui.eclipse.widgets.DashboardComposite;

/**
 * Performance tab graph
 *
 */
public class PerfTabGraph extends DashboardComposite
{
	private long nodeId;
	private List<PerfTabDci> items = new ArrayList<PerfTabDci>(4);
	private HistoricalDataChart chart;
	private ViewRefreshController refreshController = null;
	private boolean updateInProgress = false;
	private NXCSession session;
	private long timeInterval;
	private IViewPart viewPart;
	private VisibilityValidator validator;
	
	/**
	 * @param parent
	 * @param style
	 */
	public PerfTabGraph(Composite parent, long nodeId, PerfTabDci dci, PerfTabGraphSettings settings, IViewPart viewPart, VisibilityValidator validator)
	{
		super(parent, SWT.BORDER);
		this.nodeId = nodeId;
		this.viewPart = viewPart;
		this.validator = validator;
		items.add(dci);
		session = (NXCSession)ConsoleSharedData.getSession();
		
		setLayout(new FillLayout());
		
		chart = ChartFactory.createLineChart(this, SWT.NONE);
		chart.setZoomEnabled(false);
		chart.setTitleVisible(true);
		chart.setChartTitle(settings.getRuntimeTitle());
		chart.setLegendVisible(false);
		chart.setLogScaleEnabled(settings.isLogScaleEnabled());
      
		timeInterval = settings.getTimeRangeMillis();
      final Date from = new Date(System.currentTimeMillis() - settings.getTimeRangeMillis());
      final Date to = new Date(System.currentTimeMillis());
      chart.setTimeRange(from, to);
		
		GraphItemStyle style = new GraphItemStyle(settings.getType(), settings.getColorAsInt(), 2, 0);
		chart.setItemStyles(Arrays.asList(new GraphItemStyle[] { style }));
		if(!settings.isAutoScale())
      {
		   chart.setYAxisRange(settings.getMinYScaleValue(), settings.getMaxYScaleValue());
      }
		chart.addParameter(new GraphItem(nodeId, dci.getId(), 0, 0, "", settings.getRuntimeName())); //$NON-NLS-1$
		
		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (refreshController != null)
               refreshController.dispose();
         }
      });
	}
	
	/**
	 * Add another item to graph
	 * 
	 * @param dci
	 * @param settings
	 */
	public void addItem(PerfTabDci dci, PerfTabGraphSettings settings)
	{
		chart.setLegendVisible(true);
		synchronized(items)
		{
			items.add(dci);
			GraphItemStyle style = new GraphItemStyle(settings.getType(), settings.getColorAsInt(), 2, 0);
			List<GraphItemStyle> styles = new ArrayList<GraphItemStyle>(chart.getItemStyles());
			if (styles.size() < items.size())
				styles.add(style);
			else
				styles.set(items.size() - 1, style);
			chart.setItemStyles(styles);
			chart.addParameter(new GraphItem(nodeId, dci.getId(), 0, 0, "", settings.getRuntimeName())); //$NON-NLS-1$
		}
	}
	
	/**
	 * Start chart update
	 */
	public void start()
	{
		refreshController = new ViewRefreshController(viewPart, 30, new Runnable() {
			@Override
			public void run()
			{
				if (PerfTabGraph.this.isDisposed())
					return;
				
				refreshData();
			}
		}, validator);
		if (validator.isVisible())
		   refreshData();
	}

	/**
	 * Refresh graph's data
	 */
	public void refreshData()
	{
		if (updateInProgress)
			return;
		
		updateInProgress = true;
		chart.clearErrors();
		
		ConsoleJob job = new ConsoleJob(Messages.get().PerfTabGraph_JobTitle, null, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			private PerfTabDci currentDci;
			
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Date from = new Date(System.currentTimeMillis() - timeInterval);
				final Date to = new Date(System.currentTimeMillis());
				synchronized(items)
				{
					final DciData[] data = new DciData[items.size()];
					for(int i = 0; i < data.length; i++)
					{
						currentDci = items.get(i);
						data[i] = session.getCollectedData(nodeId, currentDci.getId(), from, to, 0);
					}
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							if (!((Widget)chart).isDisposed())
							{
								chart.setTimeRange(from, to);
								for(int i = 0; i < data.length; i++)
									chart.updateParameter(i, data[i], true);
							}
							updateInProgress = false;
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format(Messages.get().PerfTabGraph_JobError, currentDci.getId(), currentDci.getDescription());
			}

			@Override
			protected void jobFailureHandler()
			{
				updateInProgress = false;
				super.jobFailureHandler();
			}

			@Override
			protected IStatus createFailureStatus(Exception e)
			{
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						chart.addError(getErrorMessage());
					}
				});
				return Status.OK_STATUS;
			}
		};
		job.setUser(false);
		job.start();
	}
}
