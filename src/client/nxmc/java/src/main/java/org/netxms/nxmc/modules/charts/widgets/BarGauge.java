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
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.nxmc.modules.charts.api.DataSeries;
import org.netxms.nxmc.modules.charts.api.GaugeColorMode;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Bar gauge widget implementation
 */
public class BarGauge extends GenericGauge
{
   private static final int MAX_BAR_THICKNESS = 40;
   private static final int SCALE_TEXT_HEIGHT = 16;
   private static final int SCALE_TEXT_WIDTH = 20;

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
         scaleFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.NORMAL); //$NON-NLS-1$
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
         {
            scaleFonts[i].dispose();
         }
      }
   }

   /**
    * @see org.netxms.nxmc.modules.charts.widgets.GenericGauge#renderElement(org.eclipse.swt.graphics.GC,
    *      org.netxms.client.datacollection.ChartConfiguration, java.lang.Object, org.netxms.client.datacollection.GraphItem,
    *      org.netxms.nxmc.modules.charts.api.DataSeries, int, int, int, int)
    */
   @Override
   protected void renderElement(GC gc, ChartConfiguration config, Object renderData, GraphItem dci, DataSeries data, int x, int y, int w, int h)
   {
      Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, w - INNER_MARGIN_WIDTH * 2, h - INNER_MARGIN_HEIGHT * 2);
      gc.setAntialias(SWT.ON);

      if (config.isElementBordersVisible())
      {
         gc.setForeground(chart.getColorFromPreferences("Chart.Axis.Y.Color")); //$NON-NLS-1$
         gc.drawRectangle(rect);
         rect.x += INNER_MARGIN_WIDTH;
         rect.y += INNER_MARGIN_HEIGHT;
         rect.width -= INNER_MARGIN_WIDTH * 2;
         rect.height -= INNER_MARGIN_HEIGHT * 2;
      }

      // Draw legend
      if (config.areLabelsVisible())
      {
         gc.setForeground(chart.getColorFromPreferences("Chart.Colors.Legend")); //$NON-NLS-1$
         gc.setFont(null);
         Point legendExt = gc.textExtent(dci.getDescription());
         switch(config.getLegendPosition())
         {
            case ChartConfiguration.POSITION_TOP:
               gc.drawText(dci.getDescription(), rect.x + ((rect.width - legendExt.x) / 2), rect.y + 4, true);
               rect.y += legendExt.y + 8;
               rect.height -= legendExt.y + 8;
               break;
            case ChartConfiguration.POSITION_BOTTOM:
               rect.height -= legendExt.y + 8;
               gc.drawText(dci.getDescription(), rect.x + ((rect.width - legendExt.x) / 2), rect.y + rect.height + 4, true);
               break;
            case ChartConfiguration.POSITION_LEFT:
               gc.drawText(dci.getDescription(), rect.x + 4, rect.y + ((rect.height - legendExt.y) / 2), true);
               rect.x += legendExt.x + 8;
               rect.width -= legendExt.x + 8;
               break;
            case ChartConfiguration.POSITION_RIGHT:
               rect.width -= legendExt.x + 8;
               gc.drawText(dci.getDescription(), rect.x + rect.width + 4, rect.y + ((rect.height - legendExt.y) / 2), true);
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
         rect.x -= SCALE_TEXT_WIDTH / 2;
      }

      gc.setBackground(chart.getColorFromPreferences("Chart.Colors.PlotArea"));
      gc.setForeground(chart.getColorFromPreferences("Chart.Colors.DialScale"));
      gc.fillRectangle(rect);
      gc.drawRectangle(rect);

      rect.x += 2;
      rect.y += 2;
      rect.width -= 3;
      rect.height -= 3;

      double maxValue = config.getMaxYScaleValue();
      double minValue = config.getMinYScaleValue();
      final double pointValue = config.isTransposed() ? (maxValue - minValue) / rect.width : (maxValue - minValue) / rect.height;
      if (data.getCurrentValue() > minValue)
      {
         if (config.getGaugeColorMode() == GaugeColorMode.ZONE.getValue())
         {
            double left, right = minValue;

            if (config.getLeftRedZone() > minValue)
            {
               right = config.getLeftRedZone() + (config.getLeftYellowZone() - config.getLeftRedZone()) / 2;
               drawZone(gc, rect, minValue, right, minValue, pointValue, RED_ZONE_COLOR, YELLOW_ZONE_COLOR, config.isTransposed());
            }

            left = right;
            if (config.getRightYellowZone() < maxValue)
            {
               right = config.getLeftYellowZone() + (config.getRightYellowZone() - config.getLeftYellowZone()) / 2;
               drawZone(gc, rect, left, right, minValue, pointValue, (config.getLeftYellowZone() > minValue) ? YELLOW_ZONE_COLOR : GREEN_ZONE_COLOR, GREEN_ZONE_COLOR, config.isTransposed());

               left = right;
               right = config.getRightYellowZone() + (config.getRightRedZone() - config.getRightYellowZone()) / 2;
               if (config.getRightYellowZone() < config.getRightRedZone())
               {
                  drawZone(gc, rect, left, right, minValue, pointValue, GREEN_ZONE_COLOR, YELLOW_ZONE_COLOR, config.isTransposed());
                  left = right;
                  drawZone(gc, rect, left, maxValue, minValue, pointValue, YELLOW_ZONE_COLOR, (config.getRightRedZone() < maxValue) ? RED_ZONE_COLOR : YELLOW_ZONE_COLOR, config.isTransposed());
               }
               else if (config.getRightRedZone() < maxValue)
               {
                  drawZone(gc, rect, left, right, minValue, pointValue, GREEN_ZONE_COLOR, RED_ZONE_COLOR, config.isTransposed());
               }
            }
            else
            {
               drawZone(gc, rect, left, maxValue, minValue, pointValue, (config.getLeftYellowZone() > minValue) ? YELLOW_ZONE_COLOR : GREEN_ZONE_COLOR, GREEN_ZONE_COLOR, config.isTransposed());
            }

            double v = data.getCurrentValue();
            if (v < maxValue)
            {
               gc.setBackground(chart.getColorFromPreferences("Chart.Colors.PlotArea"));
               if (config.isTransposed())
               {
                  int points = (int)((v - minValue) / pointValue);
                  gc.fillRectangle(rect.x + points, rect.y, rect.width - points, rect.height);
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
            if (config.getGaugeColorMode() == GaugeColorMode.THRESHOLD.getValue())
               gc.setBackground(StatusDisplayInfo.getStatusColor(data.getActiveThresholdSeverity()));
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
    * @param gc
    * @param rect
    * @param startValue
    * @param endValue
    * @param pointValue
    * @param startColor
    * @param endColor
    * @param config
    */
   private void drawZone(GC gc, Rectangle rect, double startValue, double endValue, double minValue, double pointValue, RGB startColor, RGB endColor, boolean isTransposed)
   {
      int start = (int)((startValue - minValue) / pointValue);
      int points = (int)((endValue - startValue) / pointValue) + 1;
      if (isTransposed)
      {
         gc.setForeground(chart.getColorCache().create(startColor));
         gc.setBackground(chart.getColorCache().create(endColor));
         gc.fillGradientRectangle(rect.x + start, rect.y, points, rect.height, false);
      }
      else
      {
         gc.setBackground(chart.getColorCache().create(startColor));
         gc.setForeground(chart.getColorCache().create(endColor));
         int y = rect.y + rect.height - start - points + 1;
         if (y + points >= rect.y + rect.height)
            points--;
         gc.fillGradientRectangle(rect.x, y, rect.width, points, true);
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
      Color scaleColor = chart.getColorFromPreferences("Chart.Colors.DialScale");
      Color scaleTextColor = chart.getColorFromPreferences("Chart.Colors.DialScaleText");
      final Font markFont = WidgetHelper.getBestFittingFont(gc, scaleFonts, "900MM", SCALE_TEXT_WIDTH, SCALE_TEXT_HEIGHT); //$NON-NLS-1$
      gc.setFont(markFont);

      double step = getStepMagnitude(Math.max(Math.abs(minValue), Math.abs(maxValue)));
      double value = minValue;
      float pointsStep = (float)(step / pointValue);
      if (isTransposed)
      {
         for(float x = 0; x < rect.width; x += pointsStep, value += step)
         {
            if (gridVisible && (x > 0))
            {
               gc.setForeground(scaleColor);
               gc.drawLine(rect.x + (int)x, rect.y - 2, rect.x + (int)x, rect.y + rect.height + 1);
            }
            String text = DataFormatter.roundDecimalValue(value, step, 5);
            gc.setForeground(scaleTextColor);
            gc.drawText(text, rect.x + (int)x, rect.y + rect.height + 4, SWT.DRAW_TRANSPARENT);
         }
      }
      else
      {
         int textHeight = gc.textExtent("999MM").y;
         for(float y = rect.height; y > 0; y -= pointsStep, value += step)
         {
            if (gridVisible && (y < rect.height))
            {
               gc.setForeground(scaleColor);
               gc.drawLine(rect.x - 2, rect.y + (int)y, rect.x + rect.width + 1, rect.y + (int)y);
            }
            String text = DataFormatter.roundDecimalValue(value, step, 5);
            gc.setForeground(scaleTextColor);
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
