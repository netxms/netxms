/**
 * 
 */
package org.netxms.ui.android.main.activities;

import org.achartengine.ChartFactory;
import org.achartengine.GraphicalView;
import org.achartengine.model.CategorySeries;
import org.achartengine.renderer.DefaultRenderer;
import org.achartengine.renderer.XYSeriesRenderer;


/**
 * Draw pie chart activity
 */
public class DrawPieChart extends AbstractComparisonChart
{

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractComparisonChart#buildGraphView()
	 */
	@Override
	protected GraphicalView buildGraphView()
	{
		return ChartFactory.getPieChartView(this, buildDataset(), buildRenderer());
	}

	/**
	 * @return
	 */
	private DefaultRenderer buildRenderer()
	{
		DefaultRenderer renderer = new DefaultRenderer();
		renderer.setLabelsTextSize(15);
		renderer.setLegendTextSize(15);
		renderer.setMargins(new int[] { 20, 30, 15, 0 });
		renderer.setPanEnabled(false);
		renderer.setZoomEnabled(false);
		for(int color : colorList)
		{
			XYSeriesRenderer r = new XYSeriesRenderer();
			r.setColor(color | 0xFF000000);
			renderer.addSeriesRenderer(r);
		}
		return renderer;
	}
	
	/**
	 * @return
	 */
	private CategorySeries buildDataset()
	{
		CategorySeries series = new CategorySeries(graphTitle);
		for(int i = 0; i < items.length; i++)
		{
			series.add(items[i].getDescription(), values[i]);
		}
		return series;
	}
}
