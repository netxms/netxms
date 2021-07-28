/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.DataSeries;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Bar chart widget
 */
public class BarChart extends GenericComparisonChart
{
   private static final int MARGIN_WIDTH = 5;
   private static final int MARGIN_HEIGHT = 10;
   private static final int MARGIN_LABELS = 4;

   /**
    * @param parent
    * @param style
    */
   public BarChart(Chart parent)
   {
      super(parent);
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericComparisonChart#createFonts()
    */
   @Override
   protected void createFonts()
   {
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericComparisonChart#disposeFonts()
    */
   @Override
   protected void disposeFonts()
   {
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericComparisonChart#render(org.eclipse.swt.graphics.GC)
    */
   @Override
   protected void render(GC gc)
   {
      Point size = getSize();
      List<GraphItem> items = chart.getItems();
      if (items.isEmpty() || (size.x < MARGIN_WIDTH * 2) || (size.y < MARGIN_HEIGHT * 2))
         return;

      List<DataSeries> series = chart.getDataSeries();
      if (series.isEmpty())
         return;

      // Calculate vertical scale
      double minValue = series.get(0).getCurrentValue();
      double maxValue = minValue;
      for(DataSeries s : series)
      {
         if (minValue > s.getCurrentValue())
            minValue = s.getCurrentValue();
         if (maxValue < s.getCurrentValue())
            maxValue = s.getCurrentValue();
      }
      if (minValue >= 0)
      {
         maxValue = adjustRange(maxValue);
         minValue = 0;
         if (maxValue == 0) // All data at 0
            maxValue = 1;
      }
      else if (maxValue > 0)
      {
         maxValue = adjustRange(maxValue);
         minValue = -adjustRange(Math.abs(minValue));
      }
      else
      {
         maxValue = 0;
         minValue = -adjustRange(Math.abs(minValue));
      }
      
      Color axisColor = getColorFromPreferences("Chart.Axis.Y.Color"); //$NON-NLS-1$
      gc.setForeground(axisColor);

      // Value of single pixel
      double pixelValue = (maxValue - minValue) / (size.y - MARGIN_HEIGHT * 2);
      double step = getStepMagnitude(Math.max(Math.abs(minValue), Math.abs(maxValue)));
      float pointsStep = (float)(step / pixelValue);

      int baseLine; // Position of X axis
      if (minValue == 0)
         baseLine = size.y - MARGIN_HEIGHT;
      else if (maxValue == 0)
         baseLine = MARGIN_HEIGHT;
      else
         baseLine = MARGIN_HEIGHT + (int)(maxValue / pixelValue);

      // Prepare Y axis labels
      ChartConfiguration config = chart.getConfiguration();
      List<Label> labels = new ArrayList<>();
      int labelWidth = 0;
      for(double v = maxValue; v >= minValue; v -= step)
      {
         Label label = new Label(gc, config.isUseMultipliers() ? DataFormatter.roundDecimalValue(v, step, 5) : Double.toString(v));
         if (label.size.x > labelWidth)
            labelWidth = label.size.x;
         labels.add(label);
      }
      labelWidth += MARGIN_LABELS;

      // Draw X axis
      int itemWidth = (size.x - MARGIN_WIDTH * 2 - labelWidth - 1) / items.size();
      gc.drawLine(MARGIN_WIDTH + labelWidth, baseLine, size.x - MARGIN_WIDTH, baseLine);
      for(int i = 1, x = MARGIN_WIDTH + labelWidth + itemWidth; i < items.size(); i++, x += itemWidth)
         gc.drawLine(x, baseLine - 2, x, baseLine + 2);

      // Draw Y axis
      gc.drawLine(MARGIN_WIDTH + labelWidth, MARGIN_HEIGHT, MARGIN_WIDTH + labelWidth, size.y - MARGIN_HEIGHT);

      // Draw labels and grid
      gc.setLineStyle(SWT.LINE_DOT);
      Color gridColor = getColorFromPreferences("Chart.Grid.Y.Color"); //$NON-NLS-1$
      float y = MARGIN_HEIGHT;
      for(int i = 0; i < labels.size(); i++, y += pointsStep)
      {
         Label label = labels.get(i);
         gc.setForeground(axisColor);
         gc.drawText(label.text, MARGIN_WIDTH + labelWidth - label.size.x - MARGIN_LABELS, (int)y - label.size.y / 2, true);
         if (config.isGridVisible() && ((int)y != baseLine))
         {
            gc.setForeground(gridColor);
            gc.drawLine(MARGIN_WIDTH + labelWidth, (int)y, size.x - MARGIN_WIDTH, (int)y);
         }
      }
      gc.setLineStyle(SWT.LINE_SOLID);

      // Draw data blocks
      if (itemWidth >= 4)
      {
         if (config.isTranslucent())
            gc.setAlpha(127);
         int margin = Math.max(Math.min(10, itemWidth / 10), 1);
         for(int i = 0, x = MARGIN_WIDTH + labelWidth; i < items.size(); i++, x += itemWidth)
         {
            double value = series.get(i).getCurrentValue();
            if (value != 0)
            {
               int color = items.get(i).getColor();
               gc.setBackground(chart.getColorCache().create((color == -1) ? chart.getPaletteEntry(i).getRGBObject() : ColorConverter.rgbFromInt(color)));
               int h = (int)(value / pixelValue);
               gc.fillRectangle(x + margin, (value > 0) ? baseLine - h : baseLine, itemWidth - margin * 2, h);
            }
         }
      }
   }

   /**
    * Adjust upper border of current range
    * 
    * @param upper
    * @return
    */
   private static double adjustRange(double upper)
   {
      double adjustedUpper = upper;
      for(double d = 0.00001; d < 10000000000000000000.0; d *= 10)
      {
         if ((upper > d) && (upper <= d * 10))
         {
            adjustedUpper -= adjustedUpper % d;
            adjustedUpper += d;
            break;
         }
      }
      return adjustedUpper;
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

   private static class Label
   {
      String text;
      Point size;

      Label(GC gc, String text)
      {
         this.text = text;
         this.size = gc.textExtent(text);
      }
   }
}
