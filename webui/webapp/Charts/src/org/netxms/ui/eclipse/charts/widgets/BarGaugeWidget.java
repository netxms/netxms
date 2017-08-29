/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.widgets;

import java.text.DecimalFormat;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.charts.Messages;
import org.netxms.ui.eclipse.charts.api.GaugeColorMode;
import org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Bar gauge widget implementation
 */
public class BarGaugeWidget extends GaugeWidget
{
   private static final int MAX_BAR_THICKNESS = 40;
   private static final int SCALE_TEXT_HEIGHT = 16;
   private static final int SCALE_TEXT_WIDTH = 20;
   
   private Font[] scaleFonts = null;
   
   /**
    * @param parent
    * @param style
    */
   public BarGaugeWidget(Composite parent, int style)
   {
      super(parent, style);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.widgets.GaugeWidget#createFonts()
    */
   @Override
   protected void createFonts()
   {
      scaleFonts = new Font[16];
      for(int i = 0; i < scaleFonts.length; i++)
         scaleFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.NORMAL); //$NON-NLS-1$
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.widgets.GaugeWidget#disposeFonts()
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

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.charts.widgets.GaugeWidget#renderElement(org.eclipse.swt.graphics.GC, org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement, int, int, int, int)
    */
   @Override
   protected void renderElement(GC gc, DataComparisonElement dci, int x, int y, int w, int h)
   {
      Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH, y + INNER_MARGIN_HEIGHT, w - INNER_MARGIN_WIDTH * 2, h - INNER_MARGIN_HEIGHT * 2);
      gc.setAntialias(SWT.ON);
      
      if (elementBordersVisible)
      {
         gc.setForeground(getColorFromPreferences("Chart.Axis.Y.Color")); //$NON-NLS-1$
         gc.drawRectangle(rect);
         rect.x += INNER_MARGIN_WIDTH;
         rect.y += INNER_MARGIN_HEIGHT;
         rect.width -= INNER_MARGIN_WIDTH * 2;
         rect.height -= INNER_MARGIN_HEIGHT * 2;
      }

      // Draw legend
      if (legendVisible)
      {
         gc.setForeground(getColorFromPreferences("Chart.Colors.Legend")); //$NON-NLS-1$
         gc.setFont(null);
         Point legendExt = gc.textExtent(dci.getName());
         switch(legendPosition)
         {
            case GraphSettings.POSITION_TOP:
               gc.drawText(dci.getName(), rect.x + ((rect.width - legendExt.x) / 2), rect.y + 4, true);
               rect.y += legendExt.y + 8;
               rect.height -= legendExt.y + 8;
               break;
            case GraphSettings.POSITION_BOTTOM:
               rect.height -= legendExt.y + 8;
               gc.drawText(dci.getName(), rect.x + ((rect.width - legendExt.x) / 2), rect.y + rect.height + 4, true);
               break;
            case GraphSettings.POSITION_LEFT:
               gc.drawText(dci.getName(), rect.x + 4, rect.y + ((rect.height - legendExt.y) / 2), true);
               rect.x += legendExt.x + 8;
               rect.width -= legendExt.x + 8;
               break;
            case GraphSettings.POSITION_RIGHT:
               rect.width -= legendExt.x + 8;
               gc.drawText(dci.getName(), rect.x + rect.width + 4, rect.y + ((rect.height - legendExt.y) / 2), true);
               break;
         }
      }
      
      if (isVertical())
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
      
      gc.setBackground(getColorFromPreferences("Chart.Colors.PlotArea")); //$NON-NLS-1$
      gc.setForeground(getColorFromPreferences("Chart.Colors.DialScale")); //$NON-NLS-1$
      gc.fillRectangle(rect);
      gc.drawRectangle(rect);

      rect.x += 2;
      rect.y += 2;
      rect.width -= 3;
      rect.height -= 3;
      
      final double pointValue = isVertical() ? (maxValue - minValue) / rect.width : (maxValue - minValue) / rect.height;
      if (dci.getValue() > minValue)
      {
         if (colorMode == GaugeColorMode.ZONE)
         {
            double left, right = minValue;
            
            if (leftRedZone > minValue)
            {
               right = leftRedZone + (leftYellowZone - leftRedZone) / 2;
               drawZone(gc, rect, minValue, right, pointValue, RED_ZONE_COLOR, YELLOW_ZONE_COLOR);
            }
            
            left = right;
            right = leftYellowZone + (rightYellowZone - leftYellowZone) / 2;
            drawZone(gc, rect, left, right, pointValue, (leftYellowZone > minValue) ? YELLOW_ZONE_COLOR : GREEN_ZONE_COLOR, GREEN_ZONE_COLOR);
            
            left = right;
            right = rightYellowZone + (rightRedZone - rightYellowZone) / 2;
            if (rightYellowZone < rightRedZone)
            {
               drawZone(gc, rect, left, right, pointValue, GREEN_ZONE_COLOR, YELLOW_ZONE_COLOR);
               left = right;
               drawZone(gc, rect, left, maxValue, pointValue, YELLOW_ZONE_COLOR, (rightRedZone < maxValue) ? RED_ZONE_COLOR : YELLOW_ZONE_COLOR);
            }
            else if (rightRedZone < maxValue)
            {
               drawZone(gc, rect, left, right, pointValue, GREEN_ZONE_COLOR, RED_ZONE_COLOR);
            }
            
            double v = dci.getValue();
            if (v < maxValue)
            {
               gc.setBackground(getColorFromPreferences("Chart.Colors.PlotArea")); //$NON-NLS-1$
               if (isVertical())
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
            if (colorMode == GaugeColorMode.THRESHOLD)
               gc.setBackground(StatusDisplayInfo.getStatusColor(dci.getActiveThresholdSeverity()));
            else
               gc.setBackground(colors.create(customColor));
            int points = (int)((dci.getValue() - minValue) / pointValue);
            if (isVertical())
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

      drawScale(gc, rect, pointValue);
   }
   
   /**
    * Draw colored zone
    * 
    * @param gc
    * @param rect
    * @param zoneStartValue
    * @param pointValue
    * @param value
    * @param color
    * @param prevColor
    */
   private void drawZone(GC gc, Rectangle rect, double startValue, double endValue, double pointValue, RGB startColor, RGB endColor)
   {
      int start = (int)((startValue - minValue) / pointValue);
      int points = (int)((endValue - startValue) / pointValue) + 1;
      if (isVertical())
      {
         gc.setForeground(colors.create(startColor));
         gc.setBackground(colors.create(endColor));
         gc.fillGradientRectangle(rect.x + start, rect.y, points, rect.height, false);
      }
      else
      {
         gc.setBackground(colors.create(startColor));
         gc.setForeground(colors.create(endColor));
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
   private void drawScale(GC gc, Rectangle rect, double pointValue)
   {
      Color scaleColor = getColorFromPreferences("Chart.Colors.DialScale"); //$NON-NLS-1$
      Color scaleTextColor = getColorFromPreferences("Chart.Colors.DialScaleText"); //$NON-NLS-1$
      
      final Font markFont = WidgetHelper.getBestFittingFont(gc, scaleFonts, "900MM", SCALE_TEXT_WIDTH, SCALE_TEXT_HEIGHT); //$NON-NLS-1$
      gc.setFont(markFont);

      double step = getStepMagnitude(Math.max(Math.abs(minValue), Math.abs(maxValue)));
      double value = minValue;
      float pointsStep = (float)(step / pointValue);
      if (isVertical())
      {
         for(float x = 0; x < rect.width; x += pointsStep, value += step)
         {
            if (gridVisible && (x > 0))
            {
               gc.setForeground(scaleColor);
               gc.drawLine(rect.x + (int)x, rect.y - 2, rect.x + (int)x, rect.y + rect.height + 1);
            }
            String text = roundedMarkValue(value, step);
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
            String text = roundedMarkValue(value, step);
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
   
   /**
    * Get rounded value for scale mark
    * 
    * @param angle
    * @param angleValue
    * @return
    */
   private static String roundedMarkValue(double value, double step)
   {
      double absValue = Math.abs(value);
      if (absValue >= 10000000000L)
      {
         return Long.toString(Math.round(value / 1000000000)) + Messages.get().DialChartWidget_G;
      }
      else if (absValue >= 1000000000)
      {
         return new DecimalFormat("#.#").format(value / 1000000000) + Messages.get().DialChartWidget_G; //$NON-NLS-1$
      }
      else if (absValue >= 10000000)
      {
         return Long.toString(Math.round(value / 1000000)) + Messages.get().DialChartWidget_M;
      }
      else if (absValue >= 1000000)
      {
         return new DecimalFormat("#.#").format(value / 1000000) + Messages.get().DialChartWidget_M; //$NON-NLS-1$
      }
      else if (absValue >= 10000)
      {
         return Long.toString(Math.round(value / 1000)) + Messages.get().DialChartWidget_K;
      }
      else if (absValue >= 1000)
      {
         return new DecimalFormat("#.#").format(value / 1000) + Messages.get().DialChartWidget_K; //$NON-NLS-1$
      }
      else if ((absValue >= 1) && (step >= 1))
      {
         return Long.toString(Math.round(value));
      }
      else if (absValue == 0)
      {
         return "0"; //$NON-NLS-1$
      }
      else
      {
         if (step < 0.00001)
            return Double.toString(value);
         if (step < 0.0001)
            return new DecimalFormat("#.#####").format(value); //$NON-NLS-1$
         if (step < 0.001)
            return new DecimalFormat("#.####").format(value); //$NON-NLS-1$
         if (step < 0.01)
            return new DecimalFormat("#.###").format(value); //$NON-NLS-1$
         return new DecimalFormat("#.##").format(value); //$NON-NLS-1$
      }
   }
}
