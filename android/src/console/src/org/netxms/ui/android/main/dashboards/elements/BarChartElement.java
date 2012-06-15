/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import org.achartengine.GraphicalView;
import org.achartengine.chart.BarChart;
import org.achartengine.chart.BarChart.Type;
import org.achartengine.model.XYMultipleSeriesDataset;
import org.achartengine.model.XYSeries;
import org.achartengine.renderer.SimpleSeriesRenderer;
import org.achartengine.renderer.XYMultipleSeriesRenderer;
import org.netxms.client.datacollection.DciData;
import org.netxms.ui.android.main.activities.helpers.ChartDciConfig;
import org.netxms.ui.android.main.dashboards.configs.BarChartConfig;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.graphics.Paint.Align;
import android.util.Log;

/**
 * Bar chart element
 */
public class BarChartElement extends AbstractDashboardElement
{
	private BarChartConfig config;
	private BarChart chart;
	private GraphicalView chartView;
	
	/**
	 * @param context
	 * @param xmlConfig
	 */
	public BarChartElement(Context context, String xmlConfig, ClientConnectorService service)
	{
		super(context, xmlConfig, service);
		try
		{
			config = BarChartConfig.createFromXml(xmlConfig);
		}
		catch(Exception e)
		{
			Log.e("BarChartElement", "Error parsing element config", e);
			config = new BarChartConfig();
		}
		
		chart = new BarChart(buildDataset(), buildRenderer(), Type.STACKED);
		chartView = new GraphicalView(context, chart);
		addView(chartView);
		
		startRefreshTask(config.getRefreshRate());
	}

	/**
	 * @return
	 */
	private XYMultipleSeriesDataset buildDataset()
	{
		XYMultipleSeriesDataset dataset = new XYMultipleSeriesDataset();

		ChartDciConfig[] items = config.getDciList();
		for(int i = 0; i < items.length; i++)
		{
			XYSeries series = new XYSeries(items[i].getName());
			for(int j = 0; j < items.length; j++)
			{
				series.add(j + 1, 0);
			}
			dataset.addSeries(series);
		}
		
		return dataset;
	}
	
	/**
	 * @return
	 */
	private XYMultipleSeriesRenderer buildRenderer()
	{
		XYMultipleSeriesRenderer renderer = new XYMultipleSeriesRenderer();
		renderer.setAxisTitleTextSize(16);
		renderer.setChartTitleTextSize(20);
		renderer.setLabelsTextSize(15);
		renderer.setLegendTextSize(15);
		renderer.setFitLegend(true);
		renderer.setBarSpacing(0.4f);
		renderer.setShowGrid(true);
		renderer.setPanEnabled(false, false);
		renderer.setZoomEnabled(false, false);
		
		renderer.setApplyBackgroundColor(true);
		renderer.setMarginsColor(BACKGROUND_COLOR);
		renderer.setBackgroundColor(BACKGROUND_COLOR);
		renderer.setAxesColor(AXIS_COLOR);
		renderer.setGridColor(AXIS_COLOR);
		
		ChartDciConfig[] items = config.getDciList();
		for(int i = 0; i < items.length; i++)
		{
			SimpleSeriesRenderer r = new SimpleSeriesRenderer();
			int color = items[i].getColorAsInt();
			if (color == -1)
				color = DEFAULT_ITEM_COLORS[i];
			r.setColor(color | 0xFF000000);
			renderer.addSeriesRenderer(r);
			r.setDisplayChartValues(false);
		}

		renderer.setXAxisMin(0.5);
		renderer.setXAxisMax(items.length + 0.5);
		renderer.setXLabels(1);
		renderer.setYLabelsAlign(Align.RIGHT);
		renderer.clearXTextLabels();
		for(int i = 0; i < items.length; i++)
			renderer.addXTextLabel(i + 1, items[i].getName());
		
		return renderer;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.dashboards.elements.AbstractDashboardElement#refresh()
	 */
	@Override
	public void refresh()
	{
		final ChartDciConfig[] items = config.getDciList();
		if (items.length == 0)
			return;
		
		try
		{
			final DciData[] dciData = new DciData[items.length];
			for (int i = 0; i < dciData.length; i++)
			{
				dciData[i] = new DciData(0, 0);
				dciData[i] = service.getSession().getCollectedData(items[i].nodeId, items[i].dciId, null, null, 1);
			}
			
			post(new Runnable() {
				@Override
				public void run()
				{
					XYMultipleSeriesDataset dataset = chart.getDataset();
					for(int i = 0; i < dciData.length; i++)
					{
						dataset.removeSeries(i);
						XYSeries series = new XYSeries(items[i].getName());
						for(int j = 0; j < items.length; j++)
						{
							series.add(j + 1, (i == j) ? dciData[i].getLastValue().getValueAsDouble() : 0);
						}
						dataset.addSeries(i, series);
					}
					chartView.repaint();
				}
			});
		}
		catch(Exception e)
		{
			Log.e("nxclient/BarChartElement", "Exception while reading data from server", e);
		}
	}
}
