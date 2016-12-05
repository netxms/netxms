/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ScheduledExecutorService;

import org.achartengine.GraphicalView;
import org.achartengine.chart.BarChart;
import org.achartengine.chart.BarChart.Type;
import org.achartengine.model.XYMultipleSeriesDataset;
import org.achartengine.model.XYSeries;
import org.achartengine.renderer.SimpleSeriesRenderer;
import org.achartengine.renderer.XYMultipleSeriesRenderer;
import org.achartengine.renderer.XYSeriesRenderer;
import org.netxms.client.Table;
import org.netxms.ui.android.helpers.Colors;
import org.netxms.ui.android.main.dashboards.configs.TableBarChartConfig;
import org.netxms.ui.android.service.ClientConnectorService;

import android.content.Context;
import android.graphics.Paint.Align;
import android.util.Log;

/**
 * Bar chart element for table DCI
 */
public class TableBarChartElement extends AbstractDashboardElement
{
	private static final String TAG = "nxclient/TableBarChartElement";

	private TableBarChartConfig config;
	private final BarChart chart;
	private final GraphicalView chartView;
	private final XYMultipleSeriesRenderer renderer;
	private final Map<String, Integer> instanceMap = new HashMap<String, Integer>(MAX_CHART_ITEMS);

	/**
	 * @param context
	 * @param xmlConfig
	 */
	public TableBarChartElement(Context context, String xmlConfig, ClientConnectorService service, ScheduledExecutorService scheduleTaskExecutor)
	{
		super(context, xmlConfig, service, scheduleTaskExecutor);
		try
		{
			config = TableBarChartConfig.createFromXml(xmlConfig);
		}
		catch (Exception e)
		{
			Log.e(TAG, "Error parsing element config", e);
			config = new TableBarChartConfig();
		}

		renderer = buildRenderer();
		chart = new BarChart(buildDataset(), renderer, Type.STACKED);
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
		renderer.setChartTitle(config.getTitle());
		renderer.setLabelsTextSize(15);
		renderer.setLegendTextSize(15);
		renderer.setFitLegend(true);
		renderer.setShowLegend(config.isShowLegend());
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

		for (int i = 0; i < Colors.DEFAULT_ITEM_COLORS.length; i++)
		{
			XYSeriesRenderer r = new XYSeriesRenderer();
			r.setColor(Colors.DEFAULT_ITEM_COLORS[i] | 0xFF000000);
			renderer.addSeriesRenderer(r);
			r.setDisplayChartValues(false);
		}

		renderer.setXAxisMin(0.5);
		renderer.setXAxisMax(1.5);
		renderer.setXLabels(1);
		renderer.setYLabelsAlign(Align.RIGHT);
		renderer.clearXTextLabels();

		return renderer;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.dashboards.elements.AbstractDashboardElement#refresh()
	 */
	@Override
	public void refresh()
	{
		try
		{
			final Table data = service.getSession().getTableLastValues(config.getNodeId(), config.getDciId());

			post(new Runnable()
			{
				@Override
				public void run()
				{
					String instanceColumn = (config.getInstanceColumn() != null) ? config.getInstanceColumn() : "";  // FIXME
					if (instanceColumn == null)
						return;

					int icIndex = data.getColumnIndex(instanceColumn);
					int dcIndex = data.getColumnIndex(config.getDataColumn());
					if ((icIndex == -1) || (dcIndex == -1))
						return;	// at least one column is missing

					XYMultipleSeriesDataset dataset = chart.getDataset();
					for (int i = 0; i < data.getRowCount(); i++)
					{
						String instance = data.getCell(i, icIndex).getValue();
						if (instance == null)
							continue;

						double value;
						try
						{
							value = Double.parseDouble(data.getCell(i, dcIndex).getValue());
						}
						catch (NumberFormatException e)
						{
							value = 0.0;
						}

						Integer index = instanceMap.get(instance);
						if (config.isIgnoreZeroValues() && (value == 0) && (index == null))
							continue;

						XYSeries series = new XYSeries(instance);
						if (index != null)
						{
							dataset.removeSeries(index);
						}
						else
						{
							index = dataset.getSeriesCount();
							instanceMap.put(instance, index);
						}

						for (int j = 0; j < instanceMap.size(); j++)
						{
							series.add(j + 1, (index == j) ? value : 0);
						}

						dataset.addSeries(index, series);
					}

					renderer.setXAxisMax(dataset.getSeriesCount() + 0.5);
					chartView.repaint();
				}
			});
		}
		catch (Exception e)
		{
			Log.e("nxclient/BarChartElement", "Exception while reading data from server", e);
		}
	}
}
