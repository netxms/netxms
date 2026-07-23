/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets.helpers;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.dashboards.config.GaugeConfig;

/**
 * Helper class for creating gauge charts from dashboard element configuration.
 */
public final class GaugeChartHelper
{
   /**
    * Create chart widget configured according to given gauge element configuration.
    *
    * @param parent parent composite
    * @param config gauge element configuration
    * @param view owning view
    * @return created chart widget
    */
   public static Chart createChart(Composite parent, GaugeConfig config, View view)
   {
      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendVisible(false);
      chartConfig.setLegendPosition(config.getLegendPosition());
      chartConfig.setLabelsVisible(config.isShowLegend());
      chartConfig.setLabelsInside(config.isLegendInside());
      chartConfig.setTransposed(config.isVertical());
      chartConfig.setElementBordersVisible(config.isElementBordersVisible());
      chartConfig.setMinYScaleValue(config.getMinValue());
      chartConfig.setMaxYScaleValue(config.getMaxValue());
      chartConfig.setLeftYellowZone(config.getLeftYellowZone());
      chartConfig.setLeftRedZone(config.getLeftRedZone());
      chartConfig.setRightYellowZone(config.getRightYellowZone());
      chartConfig.setRightRedZone(config.getRightRedZone());
      chartConfig.setFontName(config.getFontName());
      chartConfig.setFontSize(config.getFontSize());
      chartConfig.setExpectedTextWidth(config.getExpectedTextWidth());
      chartConfig.setGaugeColorMode(config.getColorMode());

      ChartType chartType;
      switch(config.getGaugeType())
      {
         case GaugeConfig.BAR:
            chartType = ChartType.BAR_GAUGE;
            break;
         case GaugeConfig.CIRCULAR:
            chartType = ChartType.CIRCULAR_GAUGE;
            break;
         case GaugeConfig.TEXT:
            chartType = ChartType.TEXT_GAUGE;
            break;
         default:
            chartType = ChartType.DIAL_GAUGE;
            break;
      }

      Chart chart = new Chart(parent, SWT.NONE, chartType, chartConfig, view);
      chart.setDrillDownObjectId(config.getDrillDownObjectId());
      chart.setPaletteEntry(0, new ChartColor(config.getCustomColor()));
      return chart;
   }

   /**
    * Prevent instantiation.
    */
   private GaugeChartHelper()
   {
   }
}
