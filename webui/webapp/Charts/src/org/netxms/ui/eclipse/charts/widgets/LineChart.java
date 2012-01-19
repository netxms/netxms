/**
 * 
 */
package org.netxms.ui.eclipse.charts.widgets;

import java.util.Date;
import java.util.List;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.HistoricalDataChart;

/**
 * Line chart implementation
 */
public class LineChart extends GenericChart implements HistoricalDataChart
{

	public LineChart(Composite parent, int style)
	{
		super(parent, style);
	}

	@Override
	public void initializationComplete()
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setChartTitle(String title)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setTitleVisible(boolean visible)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public boolean isGridVisible()
	{
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void setGridVisible(boolean visible)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setLegendVisible(boolean visible)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void set3DModeEnabled(boolean enabled)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setLogScaleEnabled(boolean enabled)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void refresh()
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public boolean hasAxes()
	{
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void setBackgroundColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setPlotAreaColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setLegendColor(ChartColor foreground, ChartColor background)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setAxisColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setGridColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setTimeRange(Date from, Date to)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public boolean isZoomEnabled()
	{
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public void setZoomEnabled(boolean enableZoom)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public List<GraphItemStyle> getItemStyles()
	{
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void setItemStyles(List<GraphItemStyle> itemStyles)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public int addParameter(GraphItem item)
	{
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public void updateParameter(int index, DciData data, boolean updateChart)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void adjustXAxis(boolean repaint)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void adjustYAxis(boolean repaint)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void zoomIn()
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void zoomOut()
	{
		// TODO Auto-generated method stub
		
	}
}
