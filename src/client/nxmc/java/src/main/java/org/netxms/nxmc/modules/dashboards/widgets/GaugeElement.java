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
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.config.GaugeConfig;
import org.netxms.nxmc.modules.dashboards.views.DashboardView;

/**
 * Dial chart element
 */
public class GaugeElement extends ComparisonChartElement
{
	private GaugeConfig elementConfig;
	
	/**
	 * @param parent
	 * @param data
	 */
   public GaugeElement(DashboardControl parent, DashboardElement element, DashboardView view)
	{
      super(parent, element, view);

		try
		{
			elementConfig = GaugeConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			elementConfig = new GaugeConfig();
		}

      processCommonSettings(elementConfig);

		refreshInterval = elementConfig.getRefreshRate();
		
      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendVisible(false);
      chartConfig.setLegendPosition(elementConfig.getLegendPosition());
      chartConfig.setLabelsVisible(elementConfig.isShowLegend());
      chartConfig.setLabelsInside(elementConfig.isLegendInside());
      chartConfig.setTransposed(elementConfig.isVertical());
      chartConfig.setElementBordersVisible(elementConfig.isElementBordersVisible());
      chartConfig.setMinYScaleValue(elementConfig.getMinValue());
      chartConfig.setMaxYScaleValue(elementConfig.getMaxValue());
      chartConfig.setLeftYellowZone(elementConfig.getLeftYellowZone());
      chartConfig.setLeftRedZone(elementConfig.getLeftRedZone());
      chartConfig.setRightYellowZone(elementConfig.getRightYellowZone());
      chartConfig.setRightRedZone(elementConfig.getRightRedZone());
      chartConfig.setFontName(elementConfig.getFontName());
      chartConfig.setGaugeColorMode(elementConfig.getColorMode());

		switch(elementConfig.getGaugeType())
		{
         case GaugeConfig.BAR:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.GAUGE, chartConfig);
            updateThresholds = true;
            break;
			case GaugeConfig.TEXT:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.TEXT, chartConfig);
		      updateThresholds = true;
				break;
			default:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.DIAL, chartConfig);
				break;
		}
      chart.setDrillDownObjectId(elementConfig.getDrillDownObjectId());

		for(ChartDciConfig dci : elementConfig.getDciList())
         chart.addParameter(new GraphItem(dci, DataOrigin.INTERNAL, DataType.INT32));

      chart.setPaletteEntry(0, new ChartColor(elementConfig.getCustomColor()));
      chart.rebuild();

		startRefreshTimer();
	}

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.ComparisonChartElement#getDciList()
    */
	@Override
	protected ChartDciConfig[] getDciList()
	{
		return elementConfig.getDciList();
	}
}
