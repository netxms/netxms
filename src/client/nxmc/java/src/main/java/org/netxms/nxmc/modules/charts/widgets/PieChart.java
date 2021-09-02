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
package org.netxms.nxmc.modules.charts.widgets;

import java.util.List;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.nxmc.modules.charts.api.DataSeries;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Pie chart widget
 */
public class PieChart extends GenericComparisonChart
{
   private static final int MARGIN_WIDTH = 5;
   private static final int MARGIN_HEIGHT = 5;

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

      double total = 0;
      double[] values = new double[series.size()];
      for(int i = 0; i < series.size(); i++)
      {
         values[i] = series.get(i).getCurrentValue();
         total += values[i];
      }
      if (total == 0)
         return;

      double[] angularSize = new double[series.size()];
      for(int i = 0; i < values.length; i++)
         angularSize[i] = values[i] / total * 360.0;

      if (chart.getConfiguration().isTranslucent())
         gc.setAlpha(127);

      int boxSize = Math.min(size.x - MARGIN_WIDTH * 2, size.y - MARGIN_HEIGHT * 2);
      int x = (size.x - boxSize) / 2;
      int y = (size.y - boxSize) / 2;
      int startAngle = 0;
      for(int i = 0; i < values.length; i++)
      {
         int color = items.get(i).getColor();
         gc.setBackground(chart.getColorCache().create((color == -1) ? chart.getPaletteEntry(i).getRGBObject() : ColorConverter.rgbFromInt(color)));
         int sectorSize = (i == values.length - 1) ? 360 - startAngle : (int)Math.round(angularSize[i]);
         gc.fillArc(x, y, boxSize, boxSize, startAngle, sectorSize);
         startAngle += sectorSize;
      }
   }
}
