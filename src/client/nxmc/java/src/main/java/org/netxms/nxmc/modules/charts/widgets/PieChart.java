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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciValue;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Pie chart widget
 */
public class PieChart extends GenericComparisonChart
{
   private static final int MARGIN_WIDTH = 5;
   private static final int MARGIN_HEIGHT = 5;
   private static final int MARK_TEXT_HEIGHT = 20;
   private static final int MARK_TEXT_WIDTH = 30;
   private static final int MARKS_OFFSET = 10;

   private Font[] scaleFonts = null;
   private Font[] valueFonts = null;
   
   private boolean tooltipShown = false;
   private int centerX = 0;
   private int centerY = 0;
   private int boxSize = 0;
   private double total = 0;
   private List<Integer> angleArray;
   

   /**
    * @param parent
    */
   public PieChart(Chart parent)
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
      String fontName = chart.getConfiguration().getFontName();

      scaleFonts = new Font[16];
      for(int i = 0; i < scaleFonts.length; i++)
         scaleFonts[i] = new Font(getDisplay(), fontName, i + 6, SWT.NORMAL);

      valueFonts = new Font[16];
      for(int i = 0; i < valueFonts.length; i++)
         valueFonts[i] = new Font(getDisplay(), fontName, i * 2 + 6, SWT.BOLD);
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericComparisonChart#disposeFonts()
    */
   @Override
   protected void disposeFonts()
   {
      if (scaleFonts != null)
      {
         for(int i = 0; i < scaleFonts.length; i++)
            scaleFonts[i].dispose();
      }

      if (valueFonts != null)
      {
         for(int i = 0; i < valueFonts.length; i++)
            valueFonts[i].dispose();
      }
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericComparisonChart#render(org.eclipse.swt.graphics.GC)
    */
   @Override
   protected void render(GC gc)
   {
      final Font markFont = WidgetHelper.getBestFittingFont(gc, scaleFonts, "100%", MARK_TEXT_WIDTH, MARK_TEXT_HEIGHT); //$NON-NLS-1$
      gc.setFont(markFont);
      Point markSize = gc.textExtent("100%");

      Point size = getSize();
      List<ChartDciConfig> items = chart.getItems();
      if (items.isEmpty() || (size.x < MARGIN_WIDTH * 2 + MARKS_OFFSET * 2 - markSize.x * 2) || (size.y < MARGIN_HEIGHT * 2 + MARKS_OFFSET * 2 - markSize.y * 2))
         return;

      List<DataSeries> series = chart.getDataSeries();
      if (series.isEmpty())
         return;

      total = 0;
      double[] values = new double[series.size()];
      for(int i = 0; i < series.size(); i++)
      {
         values[i] = series.get(i).getCurrentValue();
         total += values[i] < 0 ? 0 : values[i];
      }
      if (total == 0)
         return;

      double[] angularSize = new double[series.size()];
      for(int i = 0; i < values.length; i++)
         angularSize[i] = (values[i] < 0 ? 0 : values[i]) / total * 360.0;

      if (chart.getConfiguration().isTranslucent())
         gc.setAlpha(127);

      Color plotAreaColor = ThemeEngine.getBackgroundColor("Chart.PlotArea");
      Color scaleColor = ThemeEngine.getForegroundColor("Chart.DialScale");
      gc.setForeground(scaleColor);

      boxSize = Math.min(size.x - MARGIN_WIDTH * 2 - MARKS_OFFSET * 2 - markSize.x * 2, size.y - MARGIN_HEIGHT * 2 - MARKS_OFFSET * 2 - markSize.y * 2);
      int x = (size.x - boxSize) / 2;
      int y = (size.y - boxSize) / 2;
      centerX = x + boxSize / 2 + 1;
      centerY = y + boxSize / 2 + 1;
      int startAngle = 0;
      List<Integer> angleArray = new ArrayList<Integer>();
      for(int i = 0; i < values.length; i++)
      {
         int color = items.get(i).getColorAsInt();
         gc.setBackground(chart.getColorCache().create((color == -1) ? chart.getPaletteEntry(i).getRGBObject() : ColorConverter.rgbFromInt(color)));
         int sectorSize = (i == values.length - 1) ? 360 - startAngle : (int)Math.round(angularSize[i]);
         gc.fillArc(x, y, boxSize, boxSize, startAngle, sectorSize);

         int pct = (int)(values[i] / total * 100.0);
         if (pct > 0)
         {
            int centerAngle = startAngle + sectorSize / 2;
            Point l1 = positionOnArc(centerX, centerY, boxSize / 2 + MARKS_OFFSET, centerAngle);
            Point l2 = positionOnArc(centerX, centerY, boxSize / 2, centerAngle);
            gc.drawLine(l1.x, l1.y, l2.x, l2.y);

            gc.setBackground(plotAreaColor);
            Point tc = positionOnArc(centerX, centerY, boxSize / 2 + MARKS_OFFSET + markSize.y, centerAngle);
            String mark = Integer.toString(pct) + "%";
            Point ext = gc.textExtent(mark);
            gc.drawText(mark, tc.x - ext.x / 2, tc.y - ext.y / 2);
         }

         startAngle += sectorSize;
         angleArray.add(startAngle);
      }
      this.angleArray = angleArray;

      if (chart.getConfiguration().isDoughnutRendering())
      {
         gc.setBackground(plotAreaColor);
         gc.setAlpha(255);
         int width = boxSize / 7;
         gc.fillArc(x + width, y + width, boxSize - width * 2, boxSize - width * 2, 0, 360);
      }

      if (chart.getConfiguration().isShowTotal())
      {
         String v = series.get(0).getDataFormatter().setFormatString(items.get(0).getDisplayFormat()).format(Double.toString(total),
               DateFormatFactory.getTimeFormatter()); 
         int innerBoxSize = boxSize - boxSize / 6;
         gc.setFont(WidgetHelper.getBestFittingFont(gc, valueFonts, "00000000", innerBoxSize, innerBoxSize));
         Point ext = gc.textExtent(v);
         if ((ext.x <= innerBoxSize) && (ext.y <= innerBoxSize))
         {
            if (!chart.getConfiguration().isDoughnutRendering())
               gc.setForeground(plotAreaColor);
            gc.setAlpha(255);
            gc.drawText(v, centerX - ext.x / 2, centerY - ext.y / 2, SWT.DRAW_TRANSPARENT);
         }
      }
   }

   /**
    * Find point coordinates on arc by given angle and radius. Angles are interpreted such that 0 degrees is at the 3 o'clock
    * position.
    * 
    * @param cx center point X coordinate
    * @param cy center point Y coordinate
    * @param radius radius
    * @param angle angle
    * @return point coordinates
    */
   private Point positionOnArc(int cx, int cy, int radius, int angle)
   {
      return new Point((int)(radius * Math.cos(Math.toRadians(angle)) + cx), (int)(radius * -Math.sin(Math.toRadians(angle)) + cy));
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
      int d = (int)Math.sqrt(Math.pow((px - centerX), 2) + Math.pow((py - centerY), 2));
      int smallCircle = boxSize - (boxSize / 7) * 2;
      double clickDegree = Math.toDegrees(Math.atan2(centerX - px, centerY - py));

      clickDegree += 90;
      if (clickDegree < 0)
         clickDegree = 360 + clickDegree;
      if (d < boxSize / 2 && (!chart.getConfiguration().isDoughnutRendering() ||  d > smallCircle /2))
      {
         for (int i = 0; i < angleArray.size(); i++)
         {
            if (angleArray.get(i) > clickDegree)
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
      }
      return null;
   }
}
