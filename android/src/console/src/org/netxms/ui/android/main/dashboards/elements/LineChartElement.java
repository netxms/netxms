/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.text.SimpleDateFormat;
import java.util.Date;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.ui.android.main.activities.helpers.ChartDciConfig;
import org.netxms.ui.android.main.dashboards.configs.LineChartConfig;
import org.netxms.ui.android.service.ClientConnectorService;
import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.LineGraphView;
import com.jjoe64.graphview.GraphView.GraphViewData;
import com.jjoe64.graphview.GraphView.GraphViewSeries;
import com.jjoe64.graphview.GraphView.GraphViewStyle;
import com.jjoe64.graphview.GraphView.LegendAlign;
import android.content.Context;
import android.util.Log;

/**
 * Bar chart element
 */
public class LineChartElement extends AbstractDashboardElement
{
	private static final String LOG_TAG = "nxclient/LineChartElement";
	
	private LineChartConfig config;
	private GraphView graphView = null;
	
	/**
	 * @param context
	 * @param xmlConfig
	 */
	public LineChartElement(Context context, String xmlConfig, ClientConnectorService service)
	{
		super(context, xmlConfig, service);
		try
		{
			config = LineChartConfig.createFromXml(xmlConfig);
		}
		catch(Exception e)
		{
			Log.e(LOG_TAG, "Error parsing element config", e);
			config = new LineChartConfig();
		}
		
		graphView = new LineGraphView(context, config.getTitle()) {
			@Override
			protected String formatLabel(double value, boolean isValueX)
			{
				if (isValueX)
				{
					SimpleDateFormat s = new SimpleDateFormat("HH:mm:ss");
					return s.format(new Date((long)value));
				}
				else
				{
					if (value <= 1E3)
						return String.format("%.0f", value);
					else if (value <= 1E6)
						return String.format("%.1f K", value/1E3);
					else if (value < 1E9)
						return String.format("%.1f M", value/1E6);
					else if (value < 1E12)
						return String.format("%.1f G", value/1E9);
					else if (value < 1E15)
						return String.format("%.1f T", value/1E12);
					return super.formatLabel(value, isValueX);
				}
			}
		};
		graphView.setShowLegend(config.isShowLegend());
		graphView.setScalable(false);
		graphView.setScrollable(false);
		graphView.setLegendAlign(LegendAlign.TOP);   
		graphView.setLegendWidth(240);  
		graphView.setBackgroundColor(0xFF000000);

		startRefreshTask(config.getRefreshRate());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.dashboards.elements.AbstractDashboardElement#refresh()
	 */
	@Override
	public void refresh()
	{
		final ChartDciConfig[] items = config.getDciList();
		Log.d(LOG_TAG, "refresh(): " + items.length + " items to load");
		if (items.length == 0)
			return;
		
		final long endTime = System.currentTimeMillis();
		final long startTime = endTime - config.getTimeRangeMillis();
		
		try
		{
			final DciData[] dciData = new DciData[items.length];
			for (int i = 0; i < dciData.length; i++)
			{
				dciData[i] = service.getSession().getCollectedData(items[i].nodeId, items[i].dciId, new Date(startTime), new Date(endTime), 0);
			}
			Log.d(LOG_TAG, "refresh(): data retrieved from server");
			
			post(new Runnable() {
				@Override
				public void run()
				{
					for(int i = 0; i < dciData.length; i++)
					{
						DciDataRow[] dciDataRow = dciData[i].getValues();
						GraphViewData[] gvData = new GraphViewData[dciDataRow.length];
						for (int j = dciDataRow.length-1, k = 0; j >= 0; j--, k++)	// dciData are reversed!
							gvData[k] = new GraphViewData(dciDataRow[j].getTimestamp().getTime(), dciDataRow[j].getValueAsDouble());
						int color = items[i].getColorAsInt();
						if (color == -1)
							color = DEFAULT_ITEM_COLORS[i];
						else
							color = swapRGB(color);
						GraphViewSeries series = new GraphViewSeries(items[i].getName(), new GraphViewStyle(color | 0xFF000000, 3), gvData);
						graphView.addOrReplaceSeries(i, series);
					}
					graphView.setViewPort(startTime, endTime - startTime + 1);
					Log.d(LOG_TAG, "refresh(): " + dciData.length + " series added; viewport set to " + startTime + "/" + (endTime - startTime + 1));
					
					if (getChildCount() == 0)
						addView(graphView);
					else
						graphView.repaint();
				}
			});
		}
		catch(Exception e)
		{
			Log.e(LOG_TAG, "Exception while reading data from server", e);
		}
	}
}
