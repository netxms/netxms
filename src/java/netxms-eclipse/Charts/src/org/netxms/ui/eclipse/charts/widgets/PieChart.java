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
package org.netxms.ui.eclipse.charts.widgets;

import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.DataSeries;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.WidgetHelper;

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

   /**
    * @param parent
    */
   public PieChart(Chart parent)
   {
      super(parent);
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
      List<GraphItem> items = chart.getItems();
      if (items.isEmpty() || (size.x < MARGIN_WIDTH * 2 + MARKS_OFFSET * 2 - markSize.x * 2) || (size.y < MARGIN_HEIGHT * 2 + MARKS_OFFSET * 2 - markSize.y * 2))
         return;

      List<DataSeries> series = chart.getDataSeries();
      if (series.isEmpty())
         return;

      double total = 0;
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

      Color plotAreaColor = chart.getColorFromPreferences("Chart.Colors.PlotArea");
      Color scaleColor = chart.getColorFromPreferences("Chart.Colors.DialScale");
      gc.setForeground(scaleColor);

      int boxSize = Math.min(size.x - MARGIN_WIDTH * 2 - MARKS_OFFSET * 2 - markSize.x * 2, size.y - MARGIN_HEIGHT * 2 - MARKS_OFFSET * 2 - markSize.y * 2);
      int x = (size.x - boxSize) / 2;
      int y = (size.y - boxSize) / 2;
      int cx = x + boxSize / 2 + 1;
      int cy = y + boxSize / 2 + 1;
      int startAngle = 0;
      for(int i = 0; i < values.length; i++)
      {
         int color = items.get(i).getColor();
         gc.setBackground(chart.getColorCache().create((color == -1) ? chart.getPaletteEntry(i).getRGBObject() : ColorConverter.rgbFromInt(color)));
         int sectorSize = (i == values.length - 1) ? 360 - startAngle : (int)Math.round(angularSize[i]);
         gc.fillArc(x, y, boxSize, boxSize, startAngle, sectorSize);

         int pct = (int)(values[i] / total * 100.0);
         if (pct > 0)
         {
            int centerAngle = startAngle + sectorSize / 2;
            Point l1 = positionOnArc(cx, cy, boxSize / 2 + MARKS_OFFSET, centerAngle);
            Point l2 = positionOnArc(cx, cy, boxSize / 2, centerAngle);
            gc.drawLine(l1.x, l1.y, l2.x, l2.y);

            gc.setBackground(plotAreaColor);
            Point tc = positionOnArc(cx, cy, boxSize / 2 + MARKS_OFFSET + markSize.y, centerAngle);
            String mark = Integer.toString(pct) + "%";
            Point ext = gc.textExtent(mark);
            gc.drawText(mark, tc.x - ext.x / 2, tc.y - ext.y / 2);
         }

         startAngle += sectorSize;
      }

      if (chart.getConfiguration().isDoughnutRendering())
      {
         gc.setBackground(plotAreaColor);
         gc.setAlpha(255);
         int width = boxSize / 7;
         gc.fillArc(x + width, y + width, boxSize - width * 2, boxSize - width * 2, 0, 360);
      }

      if (chart.getConfiguration().isShowTotal())
      {
         String v = new DataFormatter(items.get(0).getDisplayFormat(), series.get(0).getDataType(), items.get(0).getMeasurementUnit()).format(Double.toString(total), RegionalSettings.TIME_FORMATTER);
         int innerBoxSize = boxSize - boxSize / 6;
         gc.setFont(WidgetHelper.getBestFittingFont(gc, valueFonts, "00000000", innerBoxSize, innerBoxSize));
         Point ext = gc.textExtent(v);
         if ((ext.x <= innerBoxSize) && (ext.y <= innerBoxSize))
         {
            if (!chart.getConfiguration().isDoughnutRendering())
               gc.setForeground(plotAreaColor);
            gc.setAlpha(255);
            gc.drawText(v, cx - ext.x / 2, cy - ext.y / 2, SWT.DRAW_TRANSPARENT);
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
}
