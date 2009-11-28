/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.views.helpers.DCIInfo;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.swtchart.Chart;
import org.swtchart.IAxisSet;
import org.swtchart.IAxisTick;
import org.swtchart.ILegend;
import org.swtchart.ILineSeries;
import org.swtchart.ISeriesSet;
import org.swtchart.ILineSeries.PlotSymbolType;
import org.swtchart.ISeries.SeriesType;

/**
 * @author Victor
 *
 */
public class HistoryGraph extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.charts.view.history_graph";
	public static final String JOB_FAMILY = "HistoryGraphJob";
	
	private NXCSession session;
	private ArrayList<DCIInfo> items = new ArrayList<DCIInfo>(1);
	private Chart chart;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = NXMCSharedData.getInstance().getSession();
		String id = site.getSecondaryId();
		
		// Extract DCI ids from view id
		// (first field will be unique view id, so we skip it)
		String[] fields = id.split("&");
		for(int i = 1; i < fields.length; i++)
		{
			String[] subfields = fields[i].split("\\@");
			if (subfields.length == 3)
			{
				items.add(new DCIInfo(
						Long.parseLong(subfields[1], 10), 
						Long.parseLong(subfields[0], 10),
						subfields[2]));
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		chart = new Chart(parent, SWT.NONE);
		setupChart();
		
		// Request data from server
		Job job = new Job("Get DCI values history graph")
		{
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				final Date from = new Date(System.currentTimeMillis() - 3600000);
				final Date to = new Date(System.currentTimeMillis());
				final DciData[] data = new DciData[items.size()];

				try
				{
					for(int i = 0; i < items.size(); i++)
					{
						DCIInfo info = items.get(i);
						data[i] = session.getCollectedData(info.getNodeId(), info.getDciId(), from, to, 0);
					}
					status = Status.OK_STATUS;
	
					new UIJob("Update chart") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							setChartData(data);
							return Status.OK_STATUS;
						}
					}.schedule();
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
	                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
	                    "Cannot get DCI values for history graph: " + e.getMessage(), e);
				}
				return status;
			}

			/* (non-Javadoc)
			 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
			 */
			@Override
			public boolean belongsTo(Object family)
			{
				return family == HistoryGraph.JOB_FAMILY;
			}
		};
		IWorkbenchSiteProgressService siteService =
	      (IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class);
		siteService.schedule(job, 0, true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		// TODO Auto-generated method stub
	}
	
	/**
	 * Setup chart
	 */
	private void setupChart()
	{
		chart.getTitle().setVisible(false);
		
		ILegend legend = chart.getLegend();
		legend.setPosition(SWT.BOTTOM);
	}

	/**
	 * Set chart data
	 * 
	 * @param data Retrieved DCI data
	 */
	private void setChartData(final DciData[] data)
	{
		IAxisSet axisSet = chart.getAxisSet();
		IAxisTick xTick = axisSet.getXAxis(0).getTick();

		DateFormat format = new SimpleDateFormat("HH:mm");
		xTick.setFormat(format);
	
		for(int i = 0; i < data.length; i++)
			addItemToChart(items.get(i), data[i]);
		
		axisSet.adjustRange();
		chart.redraw();
	}
	
	/**
	 * Add single DCI to chart
	 * 
	 * @param data DCI data
	 */
	private void addItemToChart(final DCIInfo info, final DciData data)
	{
		final DciDataRow[] values = data.getValues();
		
		// Create series
		Date[] xSeries = new Date[values.length];
		double[] ySeries = new double[values.length];
		for(int i = 0; i < values.length; i++)
		{
			xSeries[i] = values[i].getTimestamp();
			ySeries[i] = values[i].getValueAsDouble();
		}
		
		// Add series to chart
		ISeriesSet seriesSet = chart.getSeriesSet();
		ILineSeries series = (ILineSeries)seriesSet.createSeries(SeriesType.LINE, info.getDescription());
		
		series.setAntialias(SWT.ON);
		series.setSymbolType(PlotSymbolType.NONE);
		series.setLineWidth(2);
		
		series.setXDateSeries(xSeries);
		series.setYSeries(ySeries);
	}
}
