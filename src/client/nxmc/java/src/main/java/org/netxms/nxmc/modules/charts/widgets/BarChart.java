/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciValue;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Bar chart widget
 */
public class BarChart extends GenericComparisonChart
{
   private static final int MARGIN_WIDTH = 5;
   private static final int MARGIN_HEIGHT = 10;
   private static final int MARGIN_LABELS = 4;

   private boolean tooltipShown = false;
   private double total = 0;
   private List<Rectangle> elements;

   /**
    * @param parent
    * @param style
    */
   public BarChart(Chart parent)
   {
      super(parent);

      WidgetHelper.attachMouseTrackListener(this, new MouseTrackListener() {
         @Override
         public void mouseHover(MouseEvent e)
         {
            String text = getTooltipTextAtPoint(e.x, e.y);
            if (text != null)
            {
               setToolTipText(text);
               tooltipShown = true;
            }            
         }

         @Override
         public void mouseExit(MouseEvent e)
         {   
         }

         @Override
         public void mouseEnter(MouseEvent e)
         {     
         }
      });

      WidgetHelper.attachMouseMoveListener(this, new MouseMoveListener() {
         @Override
         public void mouseMove(MouseEvent e)
         {
            if (tooltipShown)
            {
               setToolTipText(null);
               tooltipShown = false;
            }
         }
      });
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
      List<ChartDciConfig> items = chart.getItems();
      if (items.isEmpty() || (size.x < MARGIN_WIDTH * 2) || (size.y < MARGIN_HEIGHT * 2))
         return;

      List<DataSeries> series = chart.getDataSeries();
      if (series.isEmpty())
         return;

      // Calculate min and max values
      double minValue, maxValue;
      total = 0;
      if (chart.getConfiguration().isAutoScale())
      {
         minValue = series.get(0).getCurrentValue();
         maxValue = minValue;
         for(DataSeries s : series)
         {
            if (minValue > s.getCurrentValue())
               minValue = s.getCurrentValue();
            if (maxValue < s.getCurrentValue())
               maxValue = s.getCurrentValue();
            total += s.getCurrentValue();
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
      }
      else
      {
         minValue = chart.getConfiguration().getMinYScaleValue();
         maxValue = chart.getConfiguration().getMaxYScaleValue();
         for(DataSeries s : series)
         {
            total += s.getCurrentValue();
         }
      }

      if (chart.getConfiguration().isTransposed())
         renderHorizontal(gc, size, items, series, minValue, maxValue);
      else
         renderVertical(gc, size, items, series, minValue, maxValue);
   }

   /**
    * Render vertical bar chart
    *
    * @param gc graphical context
    * @param size size
    * @param items graph items
    * @param series data series
    * @param minValue adjusted minimal value
    * @param maxValue adjusted maximal value
    */
   private void renderVertical(GC gc, Point size, List<ChartDciConfig> items, List<DataSeries> series, double minValue, double maxValue)
   {
      Color axisColor = ThemeEngine.getForegroundColor("Chart.PlotArea");
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
      Color gridColor = ThemeEngine.getForegroundColor("Chart.Grid");
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

      List<Rectangle> elements = new ArrayList<Rectangle>();
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
               int color = items.get(i).getColorAsInt();
               gc.setBackground(chart.getColorCache().create((color == -1) ? chart.getPaletteEntry(i).getRGBObject() : ColorConverter.rgbFromInt(color)));
               int h = (int)Math.abs(value / pixelValue);
               gc.fillRectangle(x + margin, (value > 0) ? baseLine - h : baseLine, itemWidth - margin * 2, h);
               Rectangle r = new Rectangle(x + margin, (value > 0) ? baseLine - h : baseLine, itemWidth - margin * 2, h);           
               elements.add(r);
            }
            else
            {
               elements.add(new Rectangle());
            }
         }
      }
      this.elements = elements;
   }

   /**
    * Render horizontal bar chart
    *
    * @param gc graphical context
    * @param size size
    * @param items graph items
    * @param series data series
    * @param minValue adjusted minimal value
    * @param maxValue adjusted maximal value
    */
   private void renderHorizontal(GC gc, Point size, List<ChartDciConfig> items, List<DataSeries> series, double minValue, double maxValue)
   {
      Color axisColor = ThemeEngine.getForegroundColor("Chart.PlotArea");
      gc.setForeground(axisColor);

      // Value of single pixel
      double pixelValue = (maxValue - minValue) / (size.x - MARGIN_WIDTH * 2);
      double step = getStepMagnitude(Math.max(Math.abs(minValue), Math.abs(maxValue)));
      float pointsStep = (float)(step / pixelValue);

      int baseLine; // Position of Y axis
      if (minValue == 0)
         baseLine = MARGIN_WIDTH;
      else if (maxValue == 0)
         baseLine = size.x - MARGIN_WIDTH;
      else
         baseLine = MARGIN_WIDTH + (int)(minValue / pixelValue);

      // Prepare Y axis labels
      int labelHeight = gc.textExtent("000").y + MARGIN_LABELS;
      ChartConfiguration config = chart.getConfiguration();
      List<Label> labels = new ArrayList<>();
      for(double v = minValue; v <= maxValue; v += step)
      {
         Label label = new Label(gc, config.isUseMultipliers() ? DataFormatter.roundDecimalValue(v, step, 5) : Double.toString(v));
         labels.add(label);
      }

      // Draw Y axis
      int itemHeight = (size.y - MARGIN_HEIGHT * 2 - labelHeight - 1) / items.size();
      gc.drawLine(baseLine, MARGIN_HEIGHT, baseLine, size.y - MARGIN_HEIGHT - labelHeight);
      for(int i = 1, y = MARGIN_HEIGHT + itemHeight; i < items.size(); i++, y += itemHeight)
         gc.drawLine(baseLine - 2, y, baseLine + 2, y);

      // Draw X axis
      gc.drawLine(MARGIN_WIDTH, size.y - MARGIN_HEIGHT - labelHeight, size.x - MARGIN_WIDTH, size.y - MARGIN_HEIGHT - labelHeight);

      // Draw labels and grid
      gc.setLineStyle(SWT.LINE_DOT);
      Color gridColor = ThemeEngine.getForegroundColor("Chart.Grid");
      float x = MARGIN_WIDTH;
      for(int i = 0; i < labels.size(); i++, x += pointsStep)
      {
         Label label = labels.get(i);
         gc.setForeground(axisColor);
         gc.drawText(label.text, (int)x - label.size.x / 2, size.y - MARGIN_HEIGHT - labelHeight, true);
         if (config.isGridVisible() && ((int)x != baseLine))
         {
            gc.setForeground(gridColor);
            gc.drawLine((int)x, MARGIN_HEIGHT, (int)x, size.y - MARGIN_HEIGHT - labelHeight);
         }
      }
      gc.setLineStyle(SWT.LINE_SOLID);
      
      List<Rectangle> elements = new ArrayList<Rectangle>();
      // Draw data blocks
      if (itemHeight >= 4)
      {
         if (config.isTranslucent())
            gc.setAlpha(127);
         int margin = Math.max(Math.min(10, itemHeight / 10), 1);
         for(int i = 0, y = MARGIN_HEIGHT; i < items.size(); i++, y += itemHeight)
         {
            double value = series.get(i).getCurrentValue();
            if (value != 0)
            {
               int color = items.get(i).getColorAsInt();
               gc.setBackground(chart.getColorCache().create((color == -1) ? chart.getPaletteEntry(i).getRGBObject() : ColorConverter.rgbFromInt(color)));
               int w = (int)Math.abs(value / pixelValue);
               gc.fillRectangle(((value < 0) ? baseLine - w : baseLine) + 1, y + margin, w, itemHeight - margin * 2);
               Rectangle r = new Rectangle(((value < 0) ? baseLine - w : baseLine) + 1, y + margin, w, itemHeight - margin * 2);           
               elements.add(r);
            }
            else
            {
               elements.add(new Rectangle());
            }
         }
      }
      this.elements = elements;
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

   /**
    * Get series at given point
    * 
    * @param px current x coordinate
    * @param py current y coordinate
    * @return text of tooltip if available
    */
   private String getTooltipTextAtPoint(int px, int py)
   {
      for (int i = 0; i < elements.size(); i++)
      {
         if (elements.get(i).contains(px, py))
         {
            DataSeries s = chart.getDataSeries().get(i);
            ChartDciConfig item = chart.getItem(i);
            StringBuilder sb = new StringBuilder();
            sb.append(item.getLabel());
            sb.append("\n");
            DataFormatter df = s.getDataFormatter().setFormatString(chart.getItem(i).getDisplayFormat());
            String v = df.format(s.getCurrentValueAsString(), DateFormatFactory.getTimeFormatter());
            sb.append(v);
            sb.append(" (");
            df.setUseMultipliers(DciValue.MULTIPLIERS_NO);
            sb.append(df.format(s.getCurrentValueAsString(), DateFormatFactory.getTimeFormatter()));
            sb.append(")");
            sb.append(", ");
            int pct = (int)(s.getCurrentValue() / total * 100.0);
            sb.append(Integer.toString(pct));
            sb.append("%");
            return sb.toString();
         }
      }
      return null;
   }
}
