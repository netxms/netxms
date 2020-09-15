/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.charts.api;

import org.eclipse.swt.widgets.Composite;

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
	public static Gauge createDialChart(Composite parent, int style)
	{
		return new DialChartWidget(parent, style);
	}
	
   /**
    * Create bar gauge chart
    * 
    * @param parent parent composite
    * @param style widget style
    * @return bar gauge chart widget
    */
   public static Gauge createBarGaugeChart(Composite parent, int style)
   {
      return new BarGaugeWidget(parent, style);
   }
   
	/**
	 * Create "current values" pseudo-chart
	 * 
	 * @param parent parent composite
	 * @param style widget style
	 * @return dial chart widget
	 */
	public static Gauge createCurrentValueChart(Composite parent, int style)
	{
		return new CurrentValueWidget(parent, style);
	}
}
