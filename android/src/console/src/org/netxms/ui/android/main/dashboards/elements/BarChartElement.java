/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import org.achartengine.ChartFactory;
import org.achartengine.chart.BarChart.Type;
import org.achartengine.model.XYMultipleSeriesDataset;
import org.achartengine.model.XYSeries;
import org.achartengine.renderer.SimpleSeriesRenderer;
import org.achartengine.renderer.XYMultipleSeriesRenderer;
import org.netxms.ui.android.main.dashboards.configs.BarChartConfig;
import android.content.Context;
import android.graphics.Paint.Align;
import android.util.Log;

/**
 * Bar chart element
 */
public class BarChartElement extends AbstractDashboardElement
{
	/**
	 * @param context
	 * @param xmlConfig
	 */
	public BarChartElement(Context context, String xmlConfig)
	{
		super(context, xmlConfig);
		try
		{
			config = BarChartConfig.createFromXml(xmlConfig);
		}
		catch(Exception e)
		{
			Log.e("BarChartElement", "Error parsing element config", e);
			config = new BarChartConfig();
		}
		
		addView(ChartFactory.getBarChartView(getContext(), buildDataset(), buildRenderer(), Type.STACKED));
	}

	/**
	 * @return
	 */
	private XYMultipleSeriesDataset buildDataset()
	{
		XYMultipleSeriesDataset dataset = new XYMultipleSeriesDataset();

		for(int i = 0; i < 3; i++)
		{
			XYSeries series = new XYSeries("xxx");
			for(int j = 0; j < 3; j++)
			{
				series.add(j + 1, (i == j) ? 42 : 0);
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
		renderer.setPanEnabled(false);
		renderer.setZoomEnabled(false);
		
		//for(int color : colorList)
		for(int i = 0; i < 3; i++)
		{
			SimpleSeriesRenderer r = new SimpleSeriesRenderer();
			r.setColor(0xFF00FF00);
			renderer.addSeriesRenderer(r);
			r.setDisplayChartValues(false);
		}

		renderer.setXAxisMin(0.5);
		renderer.setXAxisMax(3 + 0.5);
		renderer.setXLabels(1);
		renderer.setYLabelsAlign(Align.RIGHT);
		renderer.clearXTextLabels();
//		for(int i = 0; i < items.length; i++)
//			renderer.addXTextLabel(i + 1, items[i].getDescription());
		
		return renderer;
	}
}
