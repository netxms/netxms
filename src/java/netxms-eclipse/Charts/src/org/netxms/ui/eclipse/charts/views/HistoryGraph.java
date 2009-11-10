/**
 * 
 */
package org.netxms.ui.eclipse.charts.views;

import java.awt.BasicStroke;
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
import org.jfree.chart.ChartColor;
import org.jfree.chart.ChartFactory;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYItemRenderer;
import org.jfree.data.time.Second;
import org.jfree.data.time.TimeSeries;
import org.jfree.data.time.TimeSeriesCollection;
import org.jfree.experimental.chart.swt.ChartComposite;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author Victor
 *
 */
public class HistoryGraph extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.charts.view.history_graph";
	public static final String JOB_FAMILY = "HistoryGraphJob";
	
	
	/**
	 * Inner class to hold source DCI information
	 */
	class DCIInfo
	{
		private long nodeId;
		private long dciId;
		
		public DCIInfo(long nodeId, long dciId)
		{
			super();
			this.nodeId = nodeId;
			this.dciId = dciId;
		}

		/**
		 * @return the nodeId
		 */
		public long getNodeId()
		{
			return nodeId;
		}

		/**
		 * @return the dciId
		 */
		public long getDciId()
		{
			return dciId;
		}
	}
	
	private NXCSession session;
	private ArrayList<DCIInfo> items = new ArrayList<DCIInfo>(1);
	private ChartComposite frame;

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
			if (subfields.length == 2)
			{
				items.add(new DCIInfo(
						Long.parseLong(subfields[1], 10), 
						Long.parseLong(subfields[0], 10)));
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		frame = new ChartComposite(parent, SWT.NONE, null, true);

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
							frame.setChart(createChart(data));
							frame.forceRedraw();
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
	 * Create chart based on a dataset
	 */
	private JFreeChart createChart(final DciData data[])
	{
		TimeSeriesCollection  dataset =  new TimeSeriesCollection();

		for(int i = 0; i < data.length; i++)
		{
			final TimeSeries  series = new TimeSeries("First");
			DciDataRow rows[] = data[i].getValues();
			for(int j = 0; j < rows.length; j++)
			{
				series.addOrUpdate(new Second(rows[j].getTimestamp()), rows[j].getValueAsDouble());
			}
			dataset.addSeries(series);
		}
		
		JFreeChart chart = ChartFactory.createTimeSeriesChart(null, null, null, dataset, false, true, false);
		
		chart.setBackgroundPaint(new ChartColor(240, 240, 240));
		chart.setBorderVisible(false);
		
		XYPlot plot = (XYPlot)chart.getPlot();
		plot.setBackgroundPaint(new ChartColor(255, 255, 255));
		plot.setRangeGridlinePaint(new ChartColor(245, 202, 202));
		plot.setRangeGridlineStroke(new BasicStroke());
		plot.setDomainGridlinePaint(new ChartColor(245, 202, 202));
		plot.setDomainGridlineStroke(new BasicStroke());
		plot.setOutlineVisible(false);
		
		final BasicStroke seriesStroke = new BasicStroke(2);
		XYItemRenderer renderer = plot.getRenderer();
		renderer.setSeriesPaint(0, new ChartColor(0, 206, 0));
		renderer.setSeriesStroke(0, seriesStroke);
		renderer.setSeriesPaint(1, new ChartColor(192, 0, 0));
		renderer.setSeriesStroke(1, seriesStroke);
		
		return chart;
	}
}
