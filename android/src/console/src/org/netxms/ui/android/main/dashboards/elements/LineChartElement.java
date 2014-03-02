/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.Date;
import java.util.concurrent.ScheduledExecutorService;

import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.ui.android.helpers.CustomLabel;
import org.netxms.ui.android.main.activities.helpers.ChartDciConfig;
import org.netxms.ui.android.main.dashboards.configs.LineChartConfig;
import org.netxms.ui.android.service.ClientConnectorService;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GraphView.GraphViewData;
import com.jjoe64.graphview.GraphView.LegendAlign;
import com.jjoe64.graphview.GraphViewSeries;
import com.jjoe64.graphview.GraphViewSeries.GraphViewSeriesStyle;
import com.jjoe64.graphview.LineGraphView;

/**
 * Bar chart element
 */
public class LineChartElement extends AbstractDashboardElement
{
	private static final String TAG = "nxclient/LineChartElement";

	private LineChartConfig config;
	private GraphView graphView = null;
	private final SharedPreferences sp;

	/**
	 * @param context
	 * @param xmlConfig
	 */
	public LineChartElement(Context context, String xmlConfig, ClientConnectorService service, ScheduledExecutorService scheduleTaskExecutor)
	{
		super(context, xmlConfig, service, scheduleTaskExecutor);
		try
		{
			config = LineChartConfig.createFromXml(xmlConfig);
		}
		catch (Exception e)
		{
			Log.e(TAG, "Error parsing element config", e);
			config = new LineChartConfig();
		}

		sp = PreferenceManager.getDefaultSharedPreferences(context);
		graphView = new LineGraphView(context, config.getTitle());
		graphView.getGraphViewStyle().setTextSize(Integer.parseInt(sp.getString("global.graph.textsize", "10")));
		graphView.getGraphViewStyle().setLegendWidth(240);
		graphView.setCustomLabelFormatter(new CustomLabel(Integer.parseInt(sp.getString("global.multipliers", "1"))));
		graphView.setShowLegend(config.isShowLegend());
		graphView.setScalable(false);
		graphView.setScrollable(false);
		graphView.setLegendAlign(LegendAlign.TOP);
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
		Log.v(TAG, "refresh(): " + items.length + " items to load");
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
			Log.v(TAG, "refresh(): data retrieved from server");

			post(new Runnable()
			{
				@Override
				public void run()
				{
					for (int i = 0; i < dciData.length; i++)
					{
						DciDataRow[] dciDataRow = dciData[i].getValues();
						GraphViewData[] gvData = new GraphViewData[dciDataRow.length];
						for (int j = dciDataRow.length - 1, k = 0; j >= 0; j--, k++)
							// dciData are reversed!
							gvData[k] = new GraphViewData(dciDataRow[j].getTimestamp().getTime(), dciDataRow[j].getValueAsDouble());
						int color = items[i].getColorAsInt();
						if (color == -1)
							color = DEFAULT_ITEM_COLORS[i];
						else
							color = swapRGB(color);
						GraphViewSeries series = new GraphViewSeries(items[i].getName(), new GraphViewSeriesStyle(color | 0xFF000000, 3), gvData);
						graphView.addSeries(series);
					}
					graphView.setViewPort(startTime, endTime - startTime + 1);
					Log.v(TAG, "refresh(): " + dciData.length + " series added; viewport set to " + startTime + "/" + (endTime - startTime + 1));

					if (getChildCount() == 0)
						addView(graphView);
					else
						graphView.redrawAll();
				}
			});
		}
		catch (Exception e)
		{
			Log.e(TAG, "Exception while reading data from server", e);
		}
	}
}
