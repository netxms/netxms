/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.ui.eclipse.charts.api.ChartType;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.dashboard.widgets.internal.TablePieChartConfig;
import com.google.gson.Gson;

/**
 * Pie chart element for table DCI
 */
public class TablePieChartElement extends TableComparisonChartElement
{
	/**
	 * @param parent
	 * @param data
	 */
	public TablePieChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
		
		try
		{
         config = new Gson().fromJson(element.getData(), TablePieChartConfig.class);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new TablePieChartConfig();
		}
		
      processCommonSettings(config);

      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendPosition(config.getLegendPosition());
      chartConfig.setLegendVisible(config.isShowLegend());
      chartConfig.setExtendedLegend(config.isExtendedLegend());
      chartConfig.setTranslucent(config.isTranslucent());

      chart = new Chart(getContentArea(), SWT.NONE, ChartType.PIE, chartConfig);
		startRefreshTimer();
	}
}
