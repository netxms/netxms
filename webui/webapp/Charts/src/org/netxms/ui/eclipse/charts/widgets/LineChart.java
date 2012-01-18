/**
 * 
 */
package org.netxms.ui.eclipse.charts.widgets;

import java.util.Date;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.HistoricalDataChart;

/**
 * Line chart implementation
 */
public class LineChart extends GenericChart implements HistoricalDataChart
{

	/**
	 * @param parent
	 * @param style
	 */
	public LineChart(Composite parent, int style)
	{
		super(parent, style);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#initializationComplete()
	 */
	@Override
	public void initializationComplete()
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setChartTitle(java.lang.String)
	 */
	@Override
	public void setChartTitle(String title)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTitleVisible(boolean)
	 */
	@Override
	public void setTitleVisible(boolean visible)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendVisible(boolean)
	 */
	@Override
	public void setLegendVisible(boolean visible)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#set3DModeEnabled(boolean)
	 */
	@Override
	public void set3DModeEnabled(boolean enabled)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLogScaleEnabled(boolean)
	 */
	@Override
	public void setLogScaleEnabled(boolean enabled)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#refresh()
	 */
	@Override
	public void refresh()
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#hasAxes()
	 */
	@Override
	public boolean hasAxes()
	{
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#setTimeRange(java.util.Date, java.util.Date)
	 */
	@Override
	public void setTimeRange(Date from, Date to)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#addParameter(org.netxms.client.datacollection.GraphItem)
	 */
	@Override
	public int addParameter(GraphItem item)
	{
		// TODO Auto-generated method stub
		return 0;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#updateParameter(int, org.netxms.client.datacollection.DciData, boolean)
	 */
	@Override
	public void updateParameter(int index, DciData data, boolean updateChart)
	{
		// TODO Auto-generated method stub

	}

}
