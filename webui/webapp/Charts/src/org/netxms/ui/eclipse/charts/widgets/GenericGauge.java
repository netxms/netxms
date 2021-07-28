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

import java.util.List;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.DataSeries;

/**
 * Abstract gauge widget
 */
public abstract class GenericGauge extends GenericComparisonChart
{
   protected static final int OUTER_MARGIN_WIDTH = 5;
   protected static final int OUTER_MARGIN_HEIGHT = 5;
   protected static final int INNER_MARGIN_WIDTH = 5;
   protected static final int INNER_MARGIN_HEIGHT = 5;

   protected static final RGB GREEN_ZONE_COLOR = new RGB(0, 224, 0);
   protected static final RGB YELLOW_ZONE_COLOR = new RGB(255, 242, 0);
   protected static final RGB RED_ZONE_COLOR = new RGB(224, 0, 0);

   /**
    * @param parent
    * @param style
    */
   public GenericGauge(Chart parent)
   {
      super(parent);
   }

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.GenericComparisonChart#render(org.eclipse.swt.graphics.GC)
    */
   @Override
   protected void render(GC gc)
   {
      Point size = getSize();
      int top = OUTER_MARGIN_HEIGHT;

      ChartConfiguration config = chart.getConfiguration();
      List<GraphItem> items = chart.getItems();
      if ((items.size() == 0) || (size.x < OUTER_MARGIN_WIDTH * 2) || (size.y < OUTER_MARGIN_HEIGHT * 2))
         return;

      List<DataSeries> series = chart.getDataSeries();
      if (config.isTransposed())
      {
         int w = (size.x - OUTER_MARGIN_WIDTH * 2);
         int h = (size.y - OUTER_MARGIN_HEIGHT - top) / items.size();
         Point minSize = getMinElementSize();
         if ((w >= minSize.x) && (h >= minSize.x))
         {
            for(int i = 0; i < items.size(); i++)
            {
               renderElement(gc, config, items.get(i), series.get(i), 0, top + i * h, w, h);
            }
         }
      }
      else
      {
         int w = (size.x - OUTER_MARGIN_WIDTH * 2) / items.size();
         int h = size.y - OUTER_MARGIN_HEIGHT - top;
         Point minSize = getMinElementSize();
         if ((w >= minSize.x) && (h >= minSize.x))
         {
            for(int i = 0; i < items.size(); i++)
            {
               renderElement(gc, config, items.get(i), series.get(i), i * w, top, w, h);
            }
         }
      }
   }

   /**
    * Get minimal element size
    * 
    * @return
    */
   protected Point getMinElementSize()
   {
      return new Point(10, 10);
   }

   /**
    * Render single gauge element.
    *
    * @param gc graphics context
    * @param configuration chart configuration
    * @param dci chart item configuration
    * @param data chart item data
    * @param x X position
    * @param y Y position
    * @param w width
    * @param h height
    */
   protected abstract void renderElement(GC gc, ChartConfiguration configuration, GraphItem dci, DataSeries data, int x, int y, int w, int h);
}
