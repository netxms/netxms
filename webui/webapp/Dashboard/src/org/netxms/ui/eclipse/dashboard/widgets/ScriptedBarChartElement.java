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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.ui.eclipse.charts.api.ChartType;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ScriptedBarChartConfig;

/**
 * Scripted bar chart element
 */
public class ScriptedBarChartElement extends ScriptedComparisonChartElement
{
   private ScriptedBarChartConfig elementConfig;
	
	/**
	 * @param parent
	 * @param data
	 */
	public ScriptedBarChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
         elementConfig = ScriptedBarChartConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
         elementConfig = new ScriptedBarChartConfig();
		}

      script = elementConfig.getScript();
      objectId = elementConfig.getObjectId();
		refreshInterval = elementConfig.getRefreshRate();

      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(elementConfig.isShowTitle());
      chartConfig.setTitle(elementConfig.getTitle());
      chartConfig.setLegendPosition(elementConfig.getLegendPosition());
      chartConfig.setLegendVisible(elementConfig.isShowLegend());
      chartConfig.setExtendedLegend(elementConfig.isExtendedLegend());
      chartConfig.setShowIn3D(elementConfig.isShowIn3D());
      chartConfig.setTransposed(elementConfig.isTransposed());
      chartConfig.setTranslucent(elementConfig.isTranslucent());
      chartConfig.setAutoScale(elementConfig.isAutoScale());
      chartConfig.setMinYScaleValue(elementConfig.getMinYScaleValue());
      chartConfig.setMaxYScaleValue(elementConfig.getMaxYScaleValue());

      chart = new Chart(this, SWT.NONE, ChartType.BAR, chartConfig);
      chart.setDrillDownObjectId(elementConfig.getDrillDownObjectId());
      chart.rebuild();

		startRefreshTimer();
	}
}
