/**
 * 
 */
package org.netxms.ui.eclipse.charts.api;

import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.charts.widgets.DataComparisonBirtChart;
import org.netxms.ui.eclipse.charts.widgets.DialChartWidget;
import org.netxms.ui.eclipse.charts.widgets.LineChart;

/**
 * Chart factory
 */
public class ChartFactory
{
	/**
	 * Create line chart
	 * 
	 * @param parent parent composite
	 * @param style widget style
	 * @return line chart widget
	 */
	public static HistoricalDataChart createLineChart(Composite parent, int style)
	{
		return new LineChart(parent, style);
	}

	/**
	 * Create bar chart
	 * 
	 * @param parent parent composite
	 * @param style widget style
	 * @return bar chart widget
	 */
	public static DataComparisonChart createBarChart(Composite parent, int style)
	{
		return new DataComparisonBirtChart(parent, style, DataComparisonChart.BAR_CHART);
	}

	/**
	 * Create tube chart
	 * 
	 * @param parent parent composite
	 * @param style widget style
	 * @return tube chart widget
	 */
	public static DataComparisonChart createTubeChart(Composite parent, int style)
	{
		return new DataComparisonBirtChart(parent, style, DataComparisonChart.TUBE_CHART);
	}

	/**
	 * Create pie chart
	 * 
	 * @param parent parent composite
	 * @param style widget style
	 * @return pie chart widget
	 */
	public static DataComparisonChart createPieChart(Composite parent, int style)
	{
		return new DataComparisonBirtChart(parent, style, DataComparisonChart.PIE_CHART);
	}

	/**
	 * Create dial chart
	 * 
	 * @param parent parent composite
	 * @param style widget style
	 * @return dial chart widget
	 */
	public static DialChart createDialChart(Composite parent, int style)
	{
		return new DialChartWidget(parent, style);
	}
}
