/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.swt.graphics.Font;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.config.GaugeConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * Dial chart element
 */
public class GaugeElement extends ComparisonChartElement
{
   private static final Logger logger = LoggerFactory.getLogger(GaugeElement.class);

	private GaugeConfig elementConfig;
   private Font heightCalculationFont = null;

	/**
	 * @param parent
	 * @param data
	 */
   public GaugeElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
	{
      super(parent, element, view);
      
      try
      {
         elementConfig = new Gson().fromJson(element.getData(), GaugeConfig.class);
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
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
      chartConfig.setFontSize(elementConfig.getFontSize());
      chartConfig.setExpectedTextWidth(elementConfig.getExpectedTextWidth());
      chartConfig.setGaugeColorMode(elementConfig.getColorMode());

      //updateThresholds = (elementConfig.getColorMode() == GaugeColorMode.THRESHOLD.getValue());
		switch(elementConfig.getGaugeType())
		{
         case GaugeConfig.BAR:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.BAR_GAUGE, chartConfig, view);
            break;
         case GaugeConfig.CIRCULAR:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.CIRCULAR_GAUGE, chartConfig, view);
            break;
         case GaugeConfig.TEXT:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.TEXT_GAUGE, chartConfig, view);
            break;
			default:
            chart = new Chart(getContentArea(), SWT.NONE, ChartType.DIAL_GAUGE, chartConfig, view);
				break;
		}
      chart.setDrillDownObjectId(elementConfig.getDrillDownObjectId());
      chart.setPaletteEntry(0, new ChartColor(elementConfig.getCustomColor()));

      configureMetrics();

      addDisposeListener((e) -> {
         if (heightCalculationFont != null)
            heightCalculationFont.dispose();
      });
	}

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.ComparisonChartElement#getDciList()
    */
	@Override
	protected ChartDciConfig[] getDciList()
	{
		return elementConfig.getDciList();
	}

   /**
    * @see org.netxms.nxmc.modules.dashboards.widgets.ElementWidget#getPreferredHeight()
    */
   @Override
   protected int getPreferredHeight()
   {
      if (elementConfig.getGaugeType() == GaugeConfig.BAR)
         return elementConfig.isVertical() ? 40 * elementConfig.getDciList().length : 40;

      if (elementConfig.getGaugeType() == GaugeConfig.TEXT)
      {
         if (heightCalculationFont == null)
         {
            int fontSize = elementConfig.getFontSize();
            if (fontSize == 0)
               fontSize = 24;
            heightCalculationFont = new Font(getDisplay(), elementConfig.getFontName(), fontSize, SWT.BOLD);
         }
         int h = WidgetHelper.getTextExtent(this, heightCalculationFont, "X").y + 10;
         if (elementConfig.isShowLegend())
            h += WidgetHelper.getTextExtent(this, "X").y;
         return elementConfig.isVertical() ? h * elementConfig.getDciList().length : h;
      }

      return super.getPreferredHeight();
   }
}
