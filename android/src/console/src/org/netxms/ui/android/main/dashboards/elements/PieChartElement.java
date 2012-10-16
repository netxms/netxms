/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.concurrent.ScheduledExecutorService;
import org.achartengine.ChartFactory;
import org.achartengine.GraphicalView;
import org.achartengine.model.CategorySeries;
import org.achartengine.renderer.DefaultRenderer;
import org.achartengine.renderer.SimpleSeriesRenderer;
import org.netxms.client.datacollection.DciData;
import org.netxms.ui.android.main.activities.helpers.ChartDciConfig;
import org.netxms.ui.android.main.dashboards.configs.PieChartConfig;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.util.Log;

/**
 * Pie chart element
 */
public class PieChartElement extends AbstractDashboardElement
{
	private static final String TAG = "nxclient/PieChartElement";
	
	private PieChartConfig config;
	private CategorySeries dataset;
	private GraphicalView chartView;
	
	/**
	 * @param context
	 * @param xmlConfig
	 */
	public PieChartElement(Context context, String xmlConfig, ClientConnectorService service, ScheduledExecutorService scheduleTaskExecutor)
	{
		super(context, xmlConfig, service, scheduleTaskExecutor);
		try
		{
			config = PieChartConfig.createFromXml(xmlConfig);
		}
		catch(Exception e)
		{
			Log.e(TAG, "Error parsing element config", e);
			config = new PieChartConfig();
		}
		
		dataset = buildDataset();
		chartView = ChartFactory.getPieChartView(context, dataset, buildRenderer());
		addView(chartView, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
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
	private CategorySeries buildDataset()
	{
		CategorySeries series = new CategorySeries(config.getTitle());
		ChartDciConfig[] items = config.getDciList();
		for(int i = 0; i < items.length; i++)
		{
			series.add(items[i].getName(), 0);
		}
		
		return series;
	}
	
	/**
	 * @return
	 */
	private DefaultRenderer buildRenderer()
	{
		DefaultRenderer renderer = new DefaultRenderer();
		renderer.setLabelsTextSize(15);
		renderer.setLegendTextSize(15);
		renderer.setShowLegend(config.isShowLegend());
		renderer.setMargins(new int[] { 20, 30, 15, 0 });
		renderer.setPanEnabled(false);
		renderer.setZoomEnabled(false);
		renderer.setAntialiasing(true);
		renderer.setChartTitle(config.getTitle());
		
		renderer.setApplyBackgroundColor(true);
		renderer.setBackgroundColor(BACKGROUND_COLOR);
		renderer.setAxesColor(LABEL_COLOR);
		renderer.setLabelsColor(LABEL_COLOR);
		
		ChartDciConfig[] items = config.getDciList();
		for(int i = 0; i < items.length; i++)
		{
			SimpleSeriesRenderer r = new SimpleSeriesRenderer();
			int color = items[i].getColorAsInt();
			if (color == -1)
				color = DEFAULT_ITEM_COLORS[i];
			else
				color = swapRGB(color);
			r.setColor(color | 0xFF000000);
			renderer.addSeriesRenderer(r);
			r.setDisplayChartValues(true);
		}
		
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
					for(int i = 0; i < dciData.length; i++)
					{
						dataset.set(i, items[i].getName(), dciData[i].getLastValue().getValueAsDouble());
					}
					chartView.repaint();
				}
			});
		}
		catch(Exception e)
		{
			Log.e(TAG, "Exception while reading data from server", e);
		}
	}
}
