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

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.nxmc.modules.charts.api.DataSeries;

/**
 * Generic plot area widget for comparison chart
 */
public abstract class GenericComparisonChart extends Canvas implements PlotArea
{
   protected Chart chart;
   protected Image chartImage = null;
   protected boolean fontsCreated = false;

   /**
    * Create chart plot area widget.
    *
    * @param parent parent chart widget
    */
   public GenericComparisonChart(Chart parent)
   {
      super(parent, SWT.NO_BACKGROUND);

      chart = parent;
      addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            if (chartImage != null)
               e.gc.drawImage(chartImage, 0, 0);
         }
      });
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (chartImage != null)
               chartImage.dispose();
            disposeFonts();
         }
      });
      addControlListener(new ControlListener() {
         @Override
         public void controlResized(ControlEvent e)
         {
            if (chartImage != null)
            {
               chartImage.dispose();
               chartImage = null;
            }
            refresh();
         }

         @Override
         public void controlMoved(ControlEvent e)
         {
         }
      });
   }

   /**
    * Create fonts
    */
   protected abstract void createFonts();

   /**
    * Dispose fonts
    */
   protected abstract void disposeFonts();

   /**
    * @see org.netxms.ui.eclipse.charts.widgets.PlotArea#refresh()
    */
   @Override
   public void refresh()
   {
      prepareGCAndRender();
      redraw();
   }

   private void prepareGCAndRender()
   {
      if (!fontsCreated)
      {
         createFonts();
         fontsCreated = true;
      }

      Point size = getSize();
      if (chartImage == null)
      {
         if ((size.x <= 0) || (size.y <= 0))
            return;
         chartImage = new Image(getDisplay(), size.x, size.y);
      }

      GC gc = new GC(chartImage);
      gc.setBackground(chart.getColorFromPreferences("Chart.Colors.Background"));
      gc.fillRectangle(0, 0, size.x, size.y);
      gc.setAntialias(SWT.ON);
      gc.setTextAntialias(SWT.ON);

      render(gc);

      gc.dispose();
   }

   /**
    * Render chart
    * 
    * @param gc graphics context
    */
   protected abstract void render(GC gc);

   /**
    * @param dci
    * @return
    */
   protected String getValueAsDisplayString(GraphItem dci, DataSeries data)
   {
      return new DataFormatter(dci.getDisplayFormat(), dci.getDataType()).format(data.getCurrentValueAsString());
   }
}
