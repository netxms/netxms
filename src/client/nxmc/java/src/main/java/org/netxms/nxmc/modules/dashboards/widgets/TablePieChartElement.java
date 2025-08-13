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
package org.netxms.nxmc.modules.dashboards.widgets;

import org.eclipse.swt.SWT;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.config.TablePieChartConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * Pie chart element for table DCI
 */
public class TablePieChartElement extends TableComparisonChartElement
{
   private static final Logger logger = LoggerFactory.getLogger(TablePieChartElement.class);
   
	/**
	 * @param parent
	 * @param data
	 */
   public TablePieChartElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);
		
		try
		{
         config = new Gson().fromJson(element.getData(), TablePieChartConfig.class);
		}
		catch(Exception e)
		{
         logger.error("Cannot parse dashboard element configuration", e);
			config = new TablePieChartConfig();
		}
		
      processCommonSettings(config);

      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendPosition(config.getLegendPosition());
      chartConfig.setLegendVisible(config.isShowLegend());
      chartConfig.setExtendedLegend(config.isExtendedLegend());
      chartConfig.setTranslucent(config.isTranslucent());

      chart = new Chart(getContentArea(), SWT.NONE, ChartType.PIE, chartConfig, view);
		startRefreshTimer();
	}
}
