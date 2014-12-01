/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.concurrent.ScheduledExecutorService;

import org.achartengine.GraphicalView;
import org.achartengine.chart.BarChart;
import org.achartengine.chart.BarChart.Type;
import org.achartengine.model.XYMultipleSeriesDataset;
import org.achartengine.model.XYSeries;
import org.achartengine.renderer.SimpleSeriesRenderer;
import org.achartengine.renderer.XYMultipleSeriesRenderer;
import org.netxms.client.datacollection.DciData;
import org.netxms.ui.android.helpers.Colors;
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
	private static final String TAG = "nxclient/BarChartElement";

	private BarChartConfig config;
	private final BarChart chart;
	private final GraphicalView chartView;

	/**
	 * @param context
	 * @param xmlConfig
	 */
	public BarChartElement(Context context, String xmlConfig, ClientConnectorService service, ScheduledExecutorService scheduleTaskExecutor)
	{
		super(context, xmlConfig, service, scheduleTaskExecutor);
		try
		{
			config = BarChartConfig.createFromXml(xmlConfig);
		}
		catch (Exception e)
		{
			Log.e(TAG, "Error parsing element config", e);
			config = new BarChartConfig();
		}

		chart = new BarChart(buildDataset(), buildRenderer(), Type.STACKED);
		chartView = new GraphicalView(context, chart);
		addView(chartView);
	}

	/* (non-Javadoc)
	 * @see android.view.View#onAttachedToWindow()
	 */
	@Override
	protected void onAttachedToWindow()
	{
		super.onAttachedToWindow();
		startRefreshTask(config.getRefreshRate());
	}

	/**
	 * @return
	 */
	private XYMultipleSeriesDataset buildDataset()
	{
		XYMultipleSeriesDataset dataset = new XYMultipleSeriesDataset();

		ChartDciConfig[] items = config.getDciList();
		for (int i = 0; i < items.length; i++)
		{
			XYSeries series = new XYSeries(items[i].getName());
			for (int j = 0; j < items.length; j++)
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
		renderer.setShowLegend(config.isShowLegend());
		renderer.setChartTitle(config.getTitle());
		renderer.setBarSpacing(0.4f);
		renderer.setShowGrid(false);
		renderer.setPanEnabled(false, false);
		renderer.setZoomEnabled(false, false);
		renderer.setAntialiasing(true);

		renderer.setApplyBackgroundColor(true);
		renderer.setMarginsColor(Colors.BACKGROUND_COLOR);
		renderer.setBackgroundColor(Colors.BACKGROUND_COLOR);
		renderer.setAxesColor(Colors.LABEL_COLOR);
		renderer.setGridColor(Colors.GRID_COLOR);
		renderer.setLabelsColor(Colors.LABEL_COLOR);
		renderer.setXLabelsColor(Colors.LABEL_COLOR);
		renderer.setYLabelsColor(0, Colors.LABEL_COLOR);

		ChartDciConfig[] items = config.getDciList();
		for (int i = 0; i < items.length && i < Colors.DEFAULT_ITEM_COLORS.length; i++)
		{
			SimpleSeriesRenderer r = new SimpleSeriesRenderer();
			int color = items[i].getColorAsInt();
			if (color == -1)
				color = Colors.DEFAULT_ITEM_COLORS[i];
			else
				color = swapRGB(color);
			r.setColor(color | 0xFF000000);
			renderer.addSeriesRenderer(r);
			r.setDisplayChartValues(false);
		}

		renderer.setXAxisMin(0.5);
		renderer.setXAxisMax(items.length + 0.5);
		renderer.setXLabelsColor(Colors.BACKGROUND_COLOR);
		renderer.setYLabelsAlign(Align.RIGHT);

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

			post(new Runnable()
			{
				@Override
				public void run()
				{
					XYMultipleSeriesDataset dataset = chart.getDataset();
					for (int i = 0; i < dciData.length; i++)
					{
						dataset.removeSeries(i);
						XYSeries series = new XYSeries(items[i].getName());
						for (int j = 0; j < items.length; j++)
						{
							series.add(j + 1, (i == j) ? dciData[i].getLastValue().getValueAsDouble() : 0);
						}
						dataset.addSeries(i, series);
					}
					chartView.repaint();
				}
			});
		}
		catch (Exception e)
		{
			Log.e(TAG, "Exception while reading data from server", e);
		}
	}
}
