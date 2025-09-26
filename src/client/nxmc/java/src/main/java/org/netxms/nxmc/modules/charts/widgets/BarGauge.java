/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.charts.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.nxmc.modules.charts.api.GaugeColorMode;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Bar gauge widget implementation
 */
public class BarGauge extends GenericGauge
{
   private static final int MAX_BAR_THICKNESS = 40;
   private static final int SCALE_TEXT_HEIGHT = 20;
   private static final int SCALE_TEXT_WIDTH = 80;

   private Font[] scaleFonts = null;

   /**
    * @param parent
    * @param style
    */
   public BarGauge(Chart parent)
   {
      super(parent);
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericGauge#createFonts()
    */
   @Override
   protected void createFonts()
   {
      String fontName = chart.getConfiguration().getFontName();
      scaleFonts = new Font[16];
      for(int i = 0; i < scaleFonts.length; i++)
         scaleFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.NORMAL);
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericGauge#disposeFonts()
    */
   @Override
   protected void disposeFonts()
   {
      if (scaleFonts != null)
      {
         for(int i = 0; i < scaleFonts.length; i++)
            scaleFonts[i].dispose();
      }
   }

   /**
    * @see org.netxms.nxmc.modules.charts.widgets.GenericGauge#renderElement(org.eclipse.swt.graphics.GC,
    *      org.netxms.client.datacollection.ChartConfiguration, java.lang.Object, org.netxms.client.datacollection.ChartDciConfig,
    *      org.netxms.nxmc.modules.charts.api.DataSeries, int, int, int, int, int)
    */
   @Override
   protected void renderElement(GC gc, ChartConfiguration config, Object renderData, ChartDciConfig dci, DataSeries data, int x, int y, int w, int h, int index)
   {
      Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, w - INNER_MARGIN_WIDTH * 2, h - INNER_MARGIN_HEIGHT * 2);
      gc.setAntialias(SWT.ON);

      // Draw legend
      if (config.areLabelsVisible())
      {
         gc.setForeground(ThemeEngine.getForegroundColor("Chart.Base"));
         gc.setFont(null);
         Point legendExt = gc.textExtent(dci.getLabel());
         switch(config.getLegendPosition())
         {
            case ChartConfiguration.POSITION_TOP:
               gc.drawText(dci.getLabel(), rect.x + ((rect.width - legendExt.x) / 2), rect.y + 4, true);
               rect.y += legendExt.y + 8;
               rect.height -= legendExt.y + 8;
               break;
            case ChartConfiguration.POSITION_BOTTOM:
               rect.height -= legendExt.y + 8;
               gc.drawText(dci.getLabel(), rect.x + ((rect.width - legendExt.x) / 2), rect.y + rect.height + 4, true);
               break;
            case ChartConfiguration.POSITION_LEFT:
               gc.drawText(dci.getLabel(), rect.x + 4, rect.y + ((rect.height - legendExt.y) / 2), true);
               rect.x += legendExt.x + 8;
               rect.width -= legendExt.x + 8;
               break;
            case ChartConfiguration.POSITION_RIGHT:
               rect.width -= legendExt.x + 8;
               gc.drawText(dci.getLabel(), rect.x + rect.width + 4, rect.y + ((rect.height - legendExt.y) / 2), true);
               break;
         }
      }

      if (config.isTransposed())
      {
         if (rect.height > MAX_BAR_THICKNESS)
         {
            int d = rect.height - MAX_BAR_THICKNESS;
            rect.height -= d;
            rect.y += d / 2;
         }
         rect.y -= SCALE_TEXT_HEIGHT / 2;
      }
      else
      {
         if (rect.width > MAX_BAR_THICKNESS)
         {
            int d = rect.width - MAX_BAR_THICKNESS;
            rect.width -= d;
            rect.x += d / 2;
         }
      }

      gc.setBackground(ThemeEngine.getBackgroundColor("Chart.PlotArea"));
      gc.fillRectangle(rect);

      double maxValue = config.getMaxYScaleValue();
      double minValue = config.getMinYScaleValue();
      final double pointValue = config.isTransposed() ? (maxValue - minValue) / rect.width : (maxValue - minValue) / rect.height;
      if (data.getCurrentValue() > minValue)
      {
         GaugeColorMode colorMode = GaugeColorMode.getByValue(config.getGaugeColorMode());
         if (colorMode == GaugeColorMode.ZONE)
         {
            drawZone(gc, rect, minValue, config.getLeftRedZone(), minValue, pointValue, RED_ZONE_COLOR, config.isTransposed());
            drawZone(gc, rect, config.getLeftRedZone(), config.getLeftYellowZone(), minValue, pointValue, YELLOW_ZONE_COLOR, config.isTransposed());
            drawZone(gc, rect, config.getLeftYellowZone(), config.getRightYellowZone(), minValue, pointValue, GREEN_ZONE_COLOR, config.isTransposed());
            drawZone(gc, rect, config.getRightYellowZone(), config.getRightRedZone(), minValue, pointValue, YELLOW_ZONE_COLOR, config.isTransposed());
            drawZone(gc, rect, config.getRightRedZone(), maxValue, minValue, pointValue, RED_ZONE_COLOR, config.isTransposed());

            double v = data.getCurrentValue();
            if (v < maxValue)
            {
               gc.setBackground(ThemeEngine.getBackgroundColor("Chart.Gauge"));
               if (config.isTransposed())
               {
                  int points = (int)((v - minValue) / pointValue);
                  gc.fillRectangle(rect.x + points, rect.y, rect.width - points + 1, rect.height);
               }
               else
               {
                  int points = (int)((maxValue - v) / pointValue);
                  gc.fillRectangle(rect.x, rect.y, rect.width, points);
               }
            }
         }
         else
         {
            gc.setBackground(ThemeEngine.getBackgroundColor("Chart.Gauge"));
            gc.fillRectangle(rect);

            if (colorMode == GaugeColorMode.THRESHOLD)
               gc.setBackground(StatusDisplayInfo.getStatusColor(data.getActiveThresholdSeverity()));
            else if (colorMode == GaugeColorMode.DATA_SOURCE)
               gc.setBackground(chart.getColorCache().create(getDataSourceColor(dci, index)));
            else
               gc.setBackground(chart.getColorCache().create(chart.getPaletteEntry(0).getRGBObject()));
            int points = (int)((data.getCurrentValue() - minValue) / pointValue);
            if (config.isTransposed())
            {
               if (points > rect.width)
                  points = rect.width;
               gc.fillRectangle(rect.x, rect.y, points, rect.height);
            }
            else
            {
               if (points > rect.height)
                  points = rect.height;
               gc.fillRectangle(rect.x, rect.y + rect.height - points, rect.width, points);
            }
         }
      }

      drawScale(gc, rect, minValue, maxValue, pointValue, config.isTransposed(), config.isGridVisible());
   }

   /**
    * Draw colored zone
    * 
    * @param gc graphic context
    * @param rect bounding rectangle
    * @param startValue start value for zone
    * @param endValue end value for zone
    * @param minValue minimal value for gauge
    * @param pointValue value of single point
    * @param color zone color
    * @param isTransposed true if gauge is transposed
    */
   private void drawZone(GC gc, Rectangle rect, double startValue, double endValue, double minValue, double pointValue, RGB color, boolean isTransposed)
   {
      float start = (float)((startValue - minValue) / pointValue);
      float points = (float)((endValue - startValue) / pointValue);
      if (isTransposed)
      {
         gc.setBackground(chart.getColorCache().create(color));
         gc.fillRectangle(rect.x + Math.round(start), rect.y, Math.round(points), rect.height);
      }
      else
      {
         gc.setBackground(chart.getColorCache().create(color));
         float y = rect.y + rect.height - start - points;
         if (y + points >= rect.y + rect.height)
            points--;
         gc.fillRectangle(rect.x, Math.round(y), rect.width, Math.round(points));
      }
   }

   /**
    * Draw bar scale
    * 
    * @param gc
    * @param rect
    * @param pointValue
    */
   private void drawScale(GC gc, Rectangle rect, double minValue, double maxValue, double pointValue, boolean isTransposed, boolean gridVisible)
   {
      gc.setForeground(ThemeEngine.getForegroundColor("Chart.DialScale"));

      final Font markFont = WidgetHelper.getBestFittingFont(gc, scaleFonts, "900MM", SCALE_TEXT_WIDTH, SCALE_TEXT_HEIGHT);
      gc.setFont(markFont);

      double step = getStepMagnitude(Math.max(Math.abs(minValue), Math.abs(maxValue)));
      double value = minValue;
      float pointsStep = (float)(step / pointValue);
      if (pointsStep < SCALE_TEXT_HEIGHT * 2)
      {
         int factor = Math.round(SCALE_TEXT_HEIGHT * 2 / pointsStep);
         pointsStep *= factor;
         step *= factor;
      }
      if (isTransposed)
      {
         for(float x = 0; x < rect.width; x += pointsStep, value += step)
         {
            String text = DataFormatter.roundDecimalValue(value, step, 5);
            gc.drawText(text, rect.x + (int)x, rect.y + rect.height + 4, SWT.DRAW_TRANSPARENT);
         }
      }
      else
      {
         int textHeight = gc.textExtent("999MM").y;
         for(float y = rect.height; y > 0; y -= pointsStep, value += step)
         {
            String text = DataFormatter.roundDecimalValue(value, step, 5);
            gc.drawText(text, rect.x + rect.width + 4, rect.y + (int)y - textHeight * 3 / 4, SWT.DRAW_TRANSPARENT);
         }
      }
   }

   private static double getStepMagnitude(double maxValue)
   {
      double d = 0.00001;
      for(; d < 10000000000000000000.0; d *= 10)
      {
         if ((maxValue > d) && (maxValue <= d * 10))
            break;
      }
      return d;
   }
}
