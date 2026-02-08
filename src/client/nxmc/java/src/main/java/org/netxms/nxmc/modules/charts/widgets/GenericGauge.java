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

import java.util.List;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Abstract gauge widget
 */
public abstract class GenericGauge extends GenericComparisonChart
{
   protected static final int OUTER_MARGIN_WIDTH = 5;
   protected static final int OUTER_MARGIN_HEIGHT = 5;
   protected static final int INNER_MARGIN_WIDTH = 5;
   protected static final int INNER_MARGIN_HEIGHT = 5;

   protected static final RGB GREEN_ZONE_COLOR = new RGB(20, 156, 74);
   protected static final RGB YELLOW_ZONE_COLOR = new RGB(253, 200, 14);
   protected static final RGB RED_ZONE_COLOR = new RGB(226, 13, 52);

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
      List<ChartDciConfig> items = chart.getItems();
      if ((items.size() == 0) || (size.x < OUTER_MARGIN_WIDTH * 2) || (size.y < OUTER_MARGIN_HEIGHT * 2))
         return;

      // top-left corner offset and width/height corrections
      int dx, dy, wc, hc;
      if (config.isElementBordersVisible())
      {
         dx = INNER_MARGIN_WIDTH;
         dy = INNER_MARGIN_HEIGHT;
         wc = -INNER_MARGIN_WIDTH * 2;
         hc = -INNER_MARGIN_HEIGHT * 2;
      }
      else
      {
         dx = dy = wc = hc = 0;
      }

      List<DataSeries> series = chart.getDataSeries();
      if (config.isTransposed())
      {
         int w = (size.x - OUTER_MARGIN_WIDTH * 2);
         int h = (size.y - OUTER_MARGIN_HEIGHT - top) / items.size();
         Point minSize = getMinElementSize();
         if ((w >= minSize.x) && (h >= minSize.x))
         {
            Object renderData = createRenderData();
            if (renderData != null)
            {
               for(int i = 0; i < items.size(); i++)
                  prepareElementRender(gc, config, renderData, items.get(i), series.get(i), dx, top + i * h + dy, w + wc, h + hc, i);
            }
            for(int i = 0; i < items.size(); i++)
            {
               renderElement(gc, config, renderData, items.get(i), series.get(i), dx, top + i * h + dy, w + wc, h + hc, i);
               if (config.isElementBordersVisible())
               {
                  gc.setForeground(ThemeEngine.getForegroundColor("Chart.PlotArea"));
                  gc.drawRectangle(0, top + i * h, w, h);
               }
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
            Object renderData = createRenderData();
            if (renderData != null)
            {
               for(int i = 0; i < items.size(); i++)
                  prepareElementRender(gc, config, renderData, items.get(i), series.get(i), i * w + dx, top + dy, w + wc, h + hc, i);
            }
            for(int i = 0; i < items.size(); i++)
            {
               renderElement(gc, config, renderData, items.get(i), series.get(i), i * w + dx, top + dy, w + wc, h + hc, i);
               if (config.isElementBordersVisible())
               {
                  gc.setForeground(ThemeEngine.getForegroundColor("Chart.PlotArea"));
                  gc.drawRectangle(i * w, top, w, h);
               }
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
    * Create gauge specific render data. Default implementation returns null.
    *
    * @return render data or null
    */
   protected Object createRenderData()
   {
      return null;
   }

   /**
    * Prepare for element rendering. Will not be called if <code>createRenderData()</code> returns null. Default implementation does
    * nothing.
    *
    * @param gc graphics context
    * @param configuration chart configuration
    * @param renderData data for rendering from <code>createRenderData()</code>
    * @param dci chart item configuration
    * @param data chart item data
    * @param x X position
    * @param y Y position
    * @param w width
    * @param h height
    * @param index element index
    */
   protected void prepareElementRender(GC gc, ChartConfiguration configuration, Object renderData, ChartDciConfig dci, DataSeries data, int x, int y, int w, int h, int index)
   {
   }

   /**
    * Render single gauge element.
    *
    * @param gc graphics context
    * @param configuration chart configuration
    * @param renderData data for rendering from preparation step
    * @param dci chart item configuration
    * @param data chart item data
    * @param x X position
    * @param y Y position
    * @param w width
    * @param h height
    * @param index element index
    */
   protected abstract void renderElement(GC gc, ChartConfiguration configuration, Object renderData, ChartDciConfig dci, DataSeries data, int x, int y, int w, int h, int index);

   /**
    * Get color for given data source.
    *
    * @param dataSource data source object
    * @param index index of this data source
    * @return color for data source
    */
   protected RGB getDataSourceColor(ChartDciConfig dataSource, int index)
   {
      int c = dataSource.getColorAsInt();
      return (c != -1) ? ColorConverter.rgbFromInt(c) : chart.getPaletteEntry(index).getRGBObject();
   }
}
