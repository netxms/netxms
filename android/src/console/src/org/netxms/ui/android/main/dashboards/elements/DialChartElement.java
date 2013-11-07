/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.concurrent.ScheduledExecutorService;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DciData;
import org.netxms.ui.android.main.activities.helpers.ChartDciConfig;
import org.netxms.ui.android.main.dashboards.configs.DialChartConfig;
import org.netxms.ui.android.main.views.DialChart;
import org.netxms.ui.android.main.views.helpers.ChartItem;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.util.Log;

/**
 * Pie chart element
 */
public class DialChartElement extends AbstractDashboardElement
{
	private static final String TAG = "nxclient/DialChartElement";
	
	private DialChartConfig config;
	private DialChart chart;
	
	/**
	 * @param context
	 * @param xmlConfig
	 */
	public DialChartElement(Context context, String xmlConfig, ClientConnectorService service, ScheduledExecutorService scheduleTaskExecutor)
	{
		super(context, xmlConfig, service, scheduleTaskExecutor);
		try
		{
			config = DialChartConfig.createFromXml(xmlConfig);
		}
		catch(Exception e)
		{
			Log.e(TAG, "Error parsing element config", e);
			config = new DialChartConfig();
		}
		
		chart = new DialChart(context);
		chart.setTitle(config.getTitle());
		chart.setTitleVisible(config.isShowTitle());
		chart.setLegendVisible(config.isShowLegend());
		chart.setLegendInside(config.isLegendInside());
		chart.setLeftRedZone(config.getLeftRedZone());
		chart.setLeftYellowZone(config.getLeftYellowZone());
		chart.setRightYellowZone(config.getRightYellowZone());
		chart.setRightRedZone(config.getRightRedZone());
		for(ChartDciConfig dci :config.getDciList())
		{
			ChartItem item = new ChartItem();
			item.nodeId = dci.nodeId;
			item.dciId = dci.dciId;
			item.name = dci.name;
			item.dataType = DataCollectionItem.DT_INT;
			item.value = 0;
			chart.addParameter(item);
		}
		
		addView(chart);
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
				dciData[i] = service.getSession().getCollectedData(items[i].nodeId, items[i].dciId, null, null, 1);
			
			post(new Runnable() {
				@Override
				public void run()
				{
					for (int i = 0; i < dciData.length; i++)
					{
						if (dciData[i] != null && dciData[i].getLastValue() != null)
							chart.updateParameter(i, dciData[i].getDataType(), dciData[i].getLastValue().getValueAsDouble());
						
					}
					chart.invalidate();
				}
			});
		}
		catch(Exception e)
		{
			Log.e(TAG, "Exception while reading data from server", e);
		}
	}
}
