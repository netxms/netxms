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
package org.netxms.nxmc.modules.dashboards.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.nxmc.modules.dashboards.config.GaugeConfig;
import org.netxms.nxmc.modules.dashboards.config.ScriptedGaugeConfig;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.dashboards.widgets.helpers.GaugeChartHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * Gauge element with data obtained from server-side script
 */
public class ScriptedGaugeElement extends ScriptedChartElement
{
   private static final Logger logger = LoggerFactory.getLogger(ScriptedGaugeElement.class);

   private ScriptedGaugeConfig elementConfig;
   private Font heightCalculationFont = null;
   private int itemCount = 1;

   /**
    * @param parent
    * @param element
    * @param view
    */
   public ScriptedGaugeElement(DashboardControl parent, DashboardElement element, AbstractDashboardView view)
   {
      super(parent, element, view);

      try
      {
         elementConfig = new Gson().fromJson(element.getData(), ScriptedGaugeConfig.class);
         if (elementConfig == null)
            elementConfig = new ScriptedGaugeConfig();
      }
      catch(Exception e)
      {
         logger.error("Cannot parse dashboard element configuration", e);
         elementConfig = new ScriptedGaugeConfig();
      }

      processCommonSettings(elementConfig);

      script = elementConfig.getScript();
      objectId = getEffectiveObjectId(elementConfig.getObjectId());
      refreshInterval = elementConfig.getRefreshRate();

      chart = GaugeChartHelper.createChart(getContentArea(), elementConfig, view);
      chart.rebuild();

      addDisposeListener((e) -> {
         if (heightCalculationFont != null)
            heightCalculationFont.dispose();
      });

      startRefreshTimer();
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.widgets.ScriptedChartElement#onDataUpdate(int)
    */
   @Override
   protected void onDataUpdate(int itemCount)
   {
      int count = Math.max(itemCount, 1);
      if (this.itemCount == count)
         return;

      // Gauges are stacked when placed vertically, so element's preferred height depends on number of items returned by script
      this.itemCount = count;
      if (elementConfig.isVertical())
         getParent().layout(true, true);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.widgets.ScriptedChartElement#getMinimumHeight()
    */
   @Override
   protected int getMinimumHeight()
   {
      return 0;   // Gauge height is controlled by getPreferredHeight()
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.widgets.ElementWidget#getPreferredHeight()
    */
   @Override
   protected int getPreferredHeight()
   {
      if (elementConfig.getGaugeType() == GaugeConfig.BAR)
         return elementConfig.isVertical() ? 40 * itemCount : 40;

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
         return elementConfig.isVertical() ? h * itemCount : h;
      }

      return super.getPreferredHeight();
   }
}
