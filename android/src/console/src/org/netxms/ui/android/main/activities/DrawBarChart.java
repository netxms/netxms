/**
 * 
 */
package org.netxms.ui.android.main.activities;

import org.achartengine.ChartFactory;
import org.achartengine.GraphicalView;
import org.achartengine.chart.BarChart.Type;
import org.achartengine.model.XYMultipleSeriesDataset;
import org.achartengine.model.XYSeries;
import org.achartengine.renderer.SimpleSeriesRenderer;
import org.achartengine.renderer.XYMultipleSeriesRenderer;
import android.graphics.Paint.Align;


/**
 * Draw pie chart activity
 */
public class DrawBarChart extends AbstractComparisonChart
{

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractComparisonChart#buildGraphView()
	 */
	@Override
	protected GraphicalView buildGraphView()
	{
		return ChartFactory.getBarChartView(this, buildDataset(), buildRenderer(), Type.STACKED);
	}
	
	/**
	 * @return
	 */
	private XYMultipleSeriesDataset buildDataset()
	{
		XYMultipleSeriesDataset dataset = new XYMultipleSeriesDataset();

		for(int i = 0; i < items.length; i++)
		{
			XYSeries series = new XYSeries(items[i].getDescription());
			for(int j = 0; j < items.length; j++)
			{
				series.add(j + 1, (i == j) ? values[i] : 0);
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
		
		for(int color : colorList)
		{
			SimpleSeriesRenderer r = new SimpleSeriesRenderer();
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
			renderer.addXTextLabel(i + 1, items[i].getDescription());
		
		return renderer;
	}
}
