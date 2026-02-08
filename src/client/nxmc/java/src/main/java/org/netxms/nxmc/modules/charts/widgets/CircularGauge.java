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
package org.netxms.nxmc.modules.charts.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.nxmc.modules.charts.api.GaugeColorMode;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Bar gauge widget implementation
 */
public class CircularGauge extends GenericGauge
{
   private static final int EXTRA_MARGIN = 10;

   private Font[] legendFonts = null;
   private Font[] valueFonts = null;

   /**
    * @param parent
    * @param style
    */
   public CircularGauge(Chart parent)
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

      legendFonts = new Font[16];
      for(int i = 0; i < legendFonts.length; i++)
         legendFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.NORMAL);

      valueFonts = new Font[16];
      for(int i = 0; i < valueFonts.length; i++)
         valueFonts[i] = new Font(getDisplay(), fontName, i * 2 + 6, SWT.BOLD);
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericGauge#disposeFonts()
    */
   @Override
   protected void disposeFonts()
   {
      if (legendFonts != null)
      {
         for(int i = 0; i < legendFonts.length; i++)
            legendFonts[i].dispose();
      }
      if (valueFonts != null)
      {
         for(int i = 0; i < valueFonts.length; i++)
            valueFonts[i].dispose();
      }
   }

   /**
    * @see org.netxms.nxmc.modules.charts.widgets.GenericGauge#renderElement(org.eclipse.swt.graphics.GC,
    *      org.netxms.client.datacollection.ChartConfiguration, java.lang.Object, org.netxms.client.datacollection.ChartDciConfig,
    *      org.netxms.nxmc.modules.charts.api.DataSeries, int, int, int, int, int)
    */
   @Override
   protected void renderElement(GC gc, ChartConfiguration configuration, Object renderData, ChartDciConfig dci, DataSeries data, int x, int y, int w, int h, int index)
   {
      Rectangle rect = new Rectangle(x + INNER_MARGIN_WIDTH + EXTRA_MARGIN, y + INNER_MARGIN_HEIGHT + EXTRA_MARGIN, w - INNER_MARGIN_WIDTH * 2 - EXTRA_MARGIN * 2,
            h - INNER_MARGIN_HEIGHT * 2 - EXTRA_MARGIN * 2);
      gc.setAntialias(SWT.ON);

      if (configuration.areLabelsVisible() && !configuration.areLabelsInside())
      {
         gc.setFont(null);
         rect.height -= gc.textExtent("MMM").y + 4; //$NON-NLS-1$
      }
      if (rect.height > rect.width)
      {
         rect.y += (rect.height - rect.width) / 2;
         rect.height = rect.width;
      }
      else
      {
         rect.x += (rect.width - rect.height) / 2;
         rect.width = rect.height;
      }

      int cx = rect.x + rect.width / 2 + 1;
      int cy = rect.y + rect.height / 2 + 1;

      double maxValue = configuration.getMaxYScaleValue();
      double minValue = configuration.getMinYScaleValue();

      double value = data.getCurrentValue();
      GaugeColorMode colorMode = GaugeColorMode.getByValue(configuration.getGaugeColorMode());
      if (colorMode == GaugeColorMode.ZONE)
      {
         double angleValue = (maxValue - minValue) / 360.0;
         double startAngle = 90.0;
         double stopAngle = 90.0 - (value - minValue) / (maxValue - minValue) * 360.0;
         startAngle = drawZone(gc, rect, startAngle, stopAngle, minValue, configuration.getLeftRedZone(), angleValue, RED_ZONE_COLOR);
         startAngle = drawZone(gc, rect, startAngle, stopAngle, configuration.getLeftRedZone(), configuration.getLeftYellowZone(), angleValue, YELLOW_ZONE_COLOR);
         startAngle = drawZone(gc, rect, startAngle, stopAngle, configuration.getLeftYellowZone(), configuration.getRightYellowZone(), angleValue, GREEN_ZONE_COLOR);
         startAngle = drawZone(gc, rect, startAngle, stopAngle, configuration.getRightYellowZone(), configuration.getRightRedZone(), angleValue, YELLOW_ZONE_COLOR);
         startAngle = drawZone(gc, rect, startAngle, stopAngle, configuration.getRightRedZone(), maxValue, angleValue, RED_ZONE_COLOR);
         gc.setBackground(ThemeEngine.getBackgroundColor("Chart.Gauge"));
         gc.fillArc(rect.x, rect.y, rect.width, rect.height, 90, -(-270 - (int)Math.ceil(startAngle)));
      }
      else
      {
         switch(colorMode)
         {
            case CUSTOM:
               gc.setBackground(chart.getColorCache().create(chart.getPaletteEntry(0).getRGBObject()));
               break;
            case DATA_SOURCE:
               gc.setBackground(chart.getColorCache().create(getDataSourceColor(dci, index)));
               break;
            case THRESHOLD:
               gc.setBackground(StatusDisplayInfo.getStatusColor(data.getActiveThresholdSeverity()));
               break;
            default:
               gc.setBackground(ThemeEngine.getForegroundColor("Chart.Gauge"));
               break;
         }

         int angularSize;
         if (value < minValue)
            angularSize = 0;
         else if (value > maxValue)
            angularSize = 360;
         else
            angularSize = (int)Math.round((value - minValue) / (maxValue - minValue) * 360.0);

         gc.fillArc(rect.x, rect.y, rect.width, rect.height, 90, -angularSize);
         gc.setBackground(ThemeEngine.getBackgroundColor("Chart.Gauge"));
         gc.fillArc(rect.x, rect.y, rect.width, rect.height, 90, 360 - angularSize);
      }
      gc.setBackground(ThemeEngine.getBackgroundColor("Chart.PlotArea"));
      int width = rect.width / 7;
      gc.fillArc(rect.x + width, rect.y + width, rect.width - width * 2, rect.height - width * 2, 0, 360);

      // Draw label and value
      int innerBoxSize = rect.width - rect.width / 6;
      if (configuration.areLabelsVisible())
      {
         gc.setFont(configuration.areLabelsInside() ? WidgetHelper.getBestFittingFont(gc, legendFonts, "XXXXXXXXXXXXXXXXXXXXXXXX", innerBoxSize, innerBoxSize) : null);
         Point ext = gc.textExtent(dci.getLabel());
         gc.setForeground(ThemeEngine.getForegroundColor("Chart.Base"));
         if (configuration.areLabelsInside())
         {
            if ((ext.x <= innerBoxSize) && (ext.y <= innerBoxSize))
               gc.drawText(dci.getLabel(), cx - ext.x / 2, cy, SWT.DRAW_TRANSPARENT);
         }
         else
         {
            gc.drawText(dci.getLabel(), rect.x + ((rect.width - ext.x) / 2), rect.y + rect.height + 4, true);
         }
      }

      String valueText = getValueAsDisplayString(dci, data);
      gc.setFont(WidgetHelper.getBestFittingFont(gc, valueFonts, "00000000", innerBoxSize, innerBoxSize));
      Point ext = gc.textExtent(valueText);
      if ((ext.x <= innerBoxSize) && (ext.y <= innerBoxSize))
      {
         switch(colorMode)
         {
            case CUSTOM:
               gc.setForeground(chart.getColorCache().create(chart.getPaletteEntry(0).getRGBObject()));
               break;
            case DATA_SOURCE:
               gc.setForeground(chart.getColorCache().create(getDataSourceColor(dci, index)));
               break;
            case THRESHOLD:
               gc.setForeground(StatusDisplayInfo.getStatusColor(data.getActiveThresholdSeverity()));
               break;
            case ZONE:
               if ((data.getCurrentValue() <= configuration.getLeftRedZone()) || (data.getCurrentValue() >= configuration.getRightRedZone()))
               {
                  gc.setForeground(chart.getColorCache().create(RED_ZONE_COLOR));
               }
               else if ((data.getCurrentValue() <= configuration.getLeftYellowZone()) || (data.getCurrentValue() >= configuration.getRightYellowZone()))
               {
                  gc.setForeground(chart.getColorCache().create(YELLOW_ZONE_COLOR));
               }
               else
               {
                  gc.setForeground(chart.getColorCache().create(GREEN_ZONE_COLOR));
               }
               break;
            default:
               gc.setForeground(ThemeEngine.getForegroundColor("Chart.DialScale"));
               break;
         }
         if (configuration.areLabelsInside())
            gc.drawText(valueText, cx - ext.x / 2, cy - ext.y, SWT.DRAW_TRANSPARENT);
         else
            gc.drawText(valueText, cx - ext.x / 2, cy - ext.y / 2, SWT.DRAW_TRANSPARENT);
      }
   }

   /**
    * Draw zone of the arc
    *
    * @param gc graphical context
    * @param rect bounding rectangle
    * @param startAngle start angle for this zone
    * @param stopAngle stop angle for colored sectors
    * @param minValue minimum value for zone
    * @param maxValue maximum value for zone
    * @param angleValue value of one degree
    * @param color zone color
    * @return start angle for next zone
    */
   private double drawZone(GC gc, Rectangle rect, double startAngle, double stopAngle, double minValue, double maxValue, double angleValue, RGB color)
   {
      if ((minValue >= maxValue) || (startAngle <= stopAngle))
         return startAngle; // Ignore incorrect zone settings or stop angle reached

      double angle = (maxValue - minValue) / angleValue;
      if (angle <= 0)
         return startAngle;
      if (startAngle - angle < stopAngle)
         angle = startAngle - stopAngle;

      gc.setBackground(chart.getColorCache().create(color));
      gc.fillArc(rect.x, rect.y, rect.width, rect.height, (int)Math.ceil(startAngle), (int)-Math.ceil(angle));
      return startAngle - angle;
   }
}
