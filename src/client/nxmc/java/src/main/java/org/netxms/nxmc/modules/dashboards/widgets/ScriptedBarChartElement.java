/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.nxmc.modules.dashboards.config.ScriptedBarChartConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * Scripted bar chart element
 */
public class ScriptedBarChartElement extends ScriptedComparisonChartElement
{
   private static final Logger logger = LoggerFactory.getLogger(ScriptedBarChartElement.class);

   private ScriptedBarChartConfig elementConfig;
	
	/**
	 * @param parent
	 * @param data
	 */
   public ScriptedBarChartElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);
      
      try
      {
         elementConfig = new Gson().fromJson(element.getData(), ScriptedBarChartConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         elementConfig = new ScriptedBarChartConfig();
      }

      processCommonSettings(elementConfig);

      script = elementConfig.getScript();
      objectId = getEffectiveObjectId(elementConfig.getObjectId());
		refreshInterval = elementConfig.getRefreshRate();

      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendPosition(elementConfig.getLegendPosition());
      chartConfig.setLegendVisible(elementConfig.isShowLegend());
      chartConfig.setExtendedLegend(elementConfig.isExtendedLegend());
      chartConfig.setTransposed(elementConfig.isTransposed());
      chartConfig.setTranslucent(elementConfig.isTranslucent());
      chartConfig.setAutoScale(elementConfig.isAutoScale());
      chartConfig.setMinYScaleValue(elementConfig.getMinYScaleValue());
      chartConfig.setMaxYScaleValue(elementConfig.getMaxYScaleValue());
      chartConfig.setYAxisLabel(elementConfig.getYAxisLabel());

      chart = new Chart(getContentArea(), SWT.NONE, ChartType.BAR, chartConfig, view);
      chart.setDrillDownObjectId(elementConfig.getDrillDownObjectId());
      chart.rebuild();

		startRefreshTimer();
	}
}
