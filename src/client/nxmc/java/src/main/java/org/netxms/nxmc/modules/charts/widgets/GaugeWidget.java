/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.DataFormatter;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.Gauge;
import org.netxms.nxmc.modules.charts.api.GaugeColorMode;
import org.netxms.nxmc.modules.charts.widgets.internal.DataComparisonElement;
import org.netxms.nxmc.tools.ColorCache;

/**
 * Abstract gauge widget
 */
public abstract class GaugeWidget extends GenericChart implements Gauge, PaintListener, DisposeListener
{
   protected static final int OUTER_MARGIN_WIDTH = 5;
   protected static final int OUTER_MARGIN_HEIGHT = 5;
   protected static final int INNER_MARGIN_WIDTH = 5;
   protected static final int INNER_MARGIN_HEIGHT = 5;

   protected static final RGB GREEN_ZONE_COLOR = new RGB(0, 224, 0);
   protected static final RGB YELLOW_ZONE_COLOR = new RGB(255, 242, 0);
   protected static final RGB RED_ZONE_COLOR = new RGB(224, 0, 0);

   protected List<DataComparisonElement> parameters = new ArrayList<DataComparisonElement>(MAX_CHART_ITEMS);
   protected Image chartImage = null;
   protected ColorCache colors;
   protected double minValue = 0.0;
   protected double maxValue = 100.0;
   protected double leftRedZone = 0.0;
   protected double leftYellowZone = 0.0;
   protected double rightYellowZone = 70.0;
   protected double rightRedZone = 90.0;
   protected boolean vertical = false;
   protected boolean legendInside = true;
   protected boolean gridVisible = true;
   protected boolean elementBordersVisible = false;
   protected String fontName = "Verdana"; //$NON-NLS-1$
   protected GaugeColorMode colorMode = GaugeColorMode.ZONE;
   protected RGB customColor = new RGB(0, 0, 0);
   protected long drillDownObjectId = 0;

   private boolean fontsCreated = false;
   private boolean mouseDown = false;

   /**
    * @param parent
    * @param style
    */
   public GaugeWidget(Composite parent, int style)
   {
      super(parent, style | SWT.NO_BACKGROUND);

      colors = new ColorCache(this);
      addPaintListener(this);
      addDisposeListener(this);
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
      addMouseListener(new MouseListener() {
         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 1)
               mouseDown = true;
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            if ((e.button == 1) && mouseDown)
            {
               mouseDown = false;
               if (drillDownObjectId != 0)
                  openDrillDownObject();
            }
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            mouseDown = false;
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
    * @see org.netxms.ui.eclipse.charts.api.DataChart#initializationComplete()
    */
   @Override
   public void initializationComplete()
   {
   }

   /**
    * Create color object from preference string
    * 
    * @param name Preference name
    * @return Color object
    */
   protected Color getColorFromPreferences(final String name)
   {
      return colors.create(preferenceStore.getAsColor(name));
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setChartTitle(java.lang.String)
    */
   @Override
   public void setChartTitle(String title)
   {
      this.title = title;
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setTitleVisible(boolean)
    */
   @Override
   public void setTitleVisible(boolean visible)
   {
      titleVisible = visible;
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setLegendVisible(boolean)
    */
   @Override
   public void setLegendVisible(boolean visible)
   {
      legendVisible = visible;
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#set3DModeEnabled(boolean)
    */
   @Override
   public void set3DModeEnabled(boolean enabled)
   {
      displayIn3D = enabled;
      refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setLogScaleEnabled(boolean)
    */
   @Override
   public void setLogScaleEnabled(boolean enabled)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#addParameter(org.netxms.client.datacollection.GraphItem, double)
    */
   @Override
   public int addParameter(GraphItem dci, double value)
   {
      parameters.add(new DataComparisonElement(dci, value, null));
      return parameters.size() - 1;
   }
   
   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#removeAllParameters()
    */
   @Override
   public void removeAllParameters()
   {
      parameters.clear();
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#updateParameter(int, double, boolean)
    */
   @Override
   public void updateParameter(int index, double value, boolean updateChart)
   {
      try
      {
			parameters.get(index).setValue(value);
      }
      catch(IndexOutOfBoundsException e)
      {
      }

      if (updateChart)
         refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#updateParameter(int, org.netxms.client.datacollection.DciDataRow,
    *      org.netxms.client.constants.DataType, boolean)
    */
   @Override
   public void updateParameter(int index, DciDataRow value, DataType dataType, boolean updateChart)
   {
      try
      {
         DataComparisonElement p = parameters.get(index);
         p.setValue(value.getValueAsDouble());
         p.setRawValue(value.getValueAsString());
         p.getObject().setDataType(dataType);
      }
      catch(IndexOutOfBoundsException e)
      {
      }

      if (updateChart)
         refresh();
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#updateParameterThresholds(int,
    *      org.netxms.client.datacollection.Threshold[])
    */
   @Override
   public void updateParameterThresholds(int index, Threshold[] thresholds)
   {
      try
      {
         DataComparisonElement p = parameters.get(index);
         p.setThresholds(thresholds);
      }
      catch(IndexOutOfBoundsException e)
      {
      }
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#setChartType(int)
    */
   @Override
   public void setChartType(int chartType)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#getChartType()
    */
   @Override
   public int getChartType()
   {
      return GAUGE_CHART;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#setTransposed(boolean)
    */
   @Override
   public void setTransposed(boolean transposed)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#isTransposed()
    */
   @Override
   public boolean isTransposed()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#setLabelsVisible(boolean)
    */
   @Override
   public void setLabelsVisible(boolean visible)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#isLabelsVisible()
    */
   @Override
   public boolean isLabelsVisible()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#setRotation(double)
    */
   @Override
   public void setRotation(double angle)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataComparisonChart#getRotation()
    */
   @Override
   public double getRotation()
   {
      return 0;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#refresh()
    */
   @Override
   public void refresh()
   {
      render();
      redraw();
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#rebuild()
    */
   @Override
   public void rebuild()
   {
      render();
      redraw();
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#hasAxes()
    */
   @Override
   public boolean hasAxes()
   {
      return false;
   }

   /**
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
   @Override
   public void paintControl(PaintEvent e)
   {
      if (chartImage != null)
         e.gc.drawImage(chartImage, 0, 0);
   }

   /**
    * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
    */
   @Override
   public void widgetDisposed(DisposeEvent e)
   {
      if (chartImage != null)
         chartImage.dispose();
      disposeFonts();
   }

   /**
    * Render chart
    */
   private void render()
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
      gc.setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$
      gc.fillRectangle(0, 0, size.x, size.y);
      gc.setAntialias(SWT.ON);
      gc.setTextAntialias(SWT.ON);

      int top = OUTER_MARGIN_HEIGHT;

      // Draw title
      if (titleVisible && (title != null))
      {
         gc.setFont(JFaceResources.getHeaderFont());
         gc.setForeground(getColorFromPreferences("Chart.Colors.Title")); //$NON-NLS-1$
         Point ext = gc.textExtent(title, SWT.DRAW_TRANSPARENT);
         int x = (ext.x < size.x) ? (size.x - ext.x) / 2 : 0;
         gc.drawText(title, x, top, true);
         top += ext.y + INNER_MARGIN_HEIGHT;
      }

      if ((parameters.size() == 0) || (size.x < OUTER_MARGIN_WIDTH * 2) || (size.y < OUTER_MARGIN_HEIGHT * 2))
      {
         gc.dispose();
         return;
      }

      if (vertical)
      {
         int w = (size.x - OUTER_MARGIN_WIDTH * 2);
         int h = (size.y - OUTER_MARGIN_HEIGHT - top) / parameters.size();
         Point minSize = getMinElementSize();
         if ((w >= minSize.x) && (h >= minSize.x))
         {
            for(int i = 0; i < parameters.size(); i++)
            {
               renderElement(gc, parameters.get(i), 0, top + i * h, w, h);
            }
         }
      }
      else
      {
         int w = (size.x - OUTER_MARGIN_WIDTH * 2) / parameters.size();
         int h = size.y - OUTER_MARGIN_HEIGHT - top;
         Point minSize = getMinElementSize();
         if ((w >= minSize.x) && (h >= minSize.x))
         {
            for(int i = 0; i < parameters.size(); i++)
            {
               renderElement(gc, parameters.get(i), i * w, top, w, h);
            }
         }
      }

      gc.dispose();
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
    * @param dataComparisonElement
    * @param i
    * @param w
    */
   protected abstract void renderElement(GC gc, DataComparisonElement dci, int x, int y, int w, int h);

   /**
    * @param dci
    * @return
    */
   protected String getValueAsDisplayString(DataComparisonElement dci)
   {
      return new DataFormatter(dci.getDisplayFormat(), dci.getObject().getDataType()).format(dci.getRawValue());
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getMinValue()
    */
   @Override
   public double getMinValue()
   {
      return minValue;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setMinValue(double)
    */
   @Override
   public void setMinValue(double minValue)
   {
      this.minValue = minValue;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getMaxValue()
    */
   @Override
   public double getMaxValue()
   {
      return maxValue;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setMaxValue(double)
    */
   @Override
   public void setMaxValue(double maxValue)
   {
      this.maxValue = maxValue;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getLeftRedZone()
    */
   @Override
   public double getLeftRedZone()
   {
      return leftRedZone;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setLeftRedZone(double)
    */
   @Override
   public void setLeftRedZone(double leftRedZone)
   {
      this.leftRedZone = leftRedZone;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getLeftYellowZone()
    */
   @Override
   public double getLeftYellowZone()
   {
      return leftYellowZone;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setLeftYellowZone(double)
    */
   @Override
   public void setLeftYellowZone(double leftYellowZone)
   {
      this.leftYellowZone = leftYellowZone;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getRightYellowZone()
    */
   @Override
   public double getRightYellowZone()
   {
      return rightYellowZone;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setRightYellowZone(double)
    */
   @Override
   public void setRightYellowZone(double rightYellowZone)
   {
      this.rightYellowZone = rightYellowZone;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getRightRedZone()
    */
   @Override
   public double getRightRedZone()
   {
      return rightRedZone;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setRightRedZone(double)
    */
   @Override
   public void setRightRedZone(double rightRedZone)
   {
      this.rightRedZone = rightRedZone;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#isLegendInside()
    */
   @Override
   public boolean isLegendInside()
   {
      return legendInside;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setLegendInside(boolean)
    */
   @Override
   public void setLegendInside(boolean legendInside)
   {
      this.legendInside = legendInside;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#isGridVisible()
    */
   @Override
   public boolean isGridVisible()
   {
      return gridVisible;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setGridVisible(boolean)
    */
   @Override
   public void setGridVisible(boolean visible)
   {
      gridVisible = visible;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setBackgroundColor(org.netxms.nxmc.modules.charts.api.ChartColor)
    */
   @Override
   public void setBackgroundColor(ChartColor color)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setPlotAreaColor(org.netxms.nxmc.modules.charts.api.ChartColor)
    */
   @Override
   public void setPlotAreaColor(ChartColor color)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setLegendColor(org.netxms.nxmc.modules.charts.api.ChartColor,
    *      org.netxms.nxmc.modules.charts.api.ChartColor)
    */
   @Override
   public void setLegendColor(ChartColor foreground, ChartColor background)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setAxisColor(org.netxms.nxmc.modules.charts.api.ChartColor)
    */
   @Override
   public void setAxisColor(ChartColor color)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setGridColor(org.netxms.nxmc.modules.charts.api.ChartColor)
    */
   @Override
   public void setGridColor(ChartColor color)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#addError(java.lang.String)
    */
   @Override
   public void addError(String message)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#clearErrors()
    */
   @Override
   public void clearErrors()
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#isVertical()
    */
   @Override
   public boolean isVertical()
   {
      return vertical;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setVertical(boolean)
    */
   @Override
   public void setVertical(boolean vertical)
   {
      this.vertical = vertical;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#isElementBordersVisible()
    */
   @Override
   public boolean isElementBordersVisible()
   {
      return elementBordersVisible;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setElementBordersVisible(boolean)
    */
   @Override
   public void setElementBordersVisible(boolean elementBordersVisible)
   {
      this.elementBordersVisible = elementBordersVisible;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getFontName()
    */
   @Override
   public String getFontName()
   {
      return fontName;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setFontName(java.lang.String)
    */
   @Override
   public void setFontName(String fontName)
   {
      if ((fontName == null) || fontName.isEmpty())
         fontName = "Verdana"; //$NON-NLS-1$
      if (!fontName.equals(this.fontName))
      {
         this.fontName = fontName;
         if (fontsCreated)
            disposeFonts();
         createFonts();
         fontsCreated = true;
      }
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#setYAxisRange(double, double)
    */
   @Override
   public void setYAxisRange(double from, double to)
   {
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getColorMode()
    */
   @Override
   public GaugeColorMode getColorMode()
   {
      return colorMode;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setColorMode(org.netxms.nxmc.modules.charts.api.GaugeColorMode)
    */
   @Override
   public void setColorMode(GaugeColorMode mode)
   {
      colorMode = mode;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getCustomColor()
    */
   @Override
   public RGB getCustomColor()
   {
      return customColor;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setCustomColor(org.eclipse.swt.graphics.RGB)
    */
   @Override
   public void setCustomColor(RGB color)
   {
      customColor = color;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#getDrillDownObjectId()
    */
   @Override
   public long getDrillDownObjectId()
   {
      return drillDownObjectId;
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.Gauge#setDrillDownObjectId(long)
    */
   @Override
   public void setDrillDownObjectId(long drillDownObjectId)
   {
      this.drillDownObjectId = drillDownObjectId;
      setCursor(getDisplay().getSystemCursor((drillDownObjectId != 0) ? SWT.CURSOR_HAND : SWT.CURSOR_ARROW));
   }
   
   /**
    * Open drill-down object
    */
   void openDrillDownObject()
   {
      AbstractObject object = Registry.getSession().findObjectById(drillDownObjectId);
      if (object == null)
         return;
      
      if (!(object instanceof Dashboard) && !(object instanceof NetworkMap))
         return;

      /* FIXME: implement open */
   }

   /**
    * @see org.netxms.nxmc.modules.charts.api.DataChart#takeSnapshot()
    */
   @Override
   public Image takeSnapshot()
   {
      Rectangle rect = getClientArea();
      Image image = new Image(getDisplay(), rect.width, rect.height);
      GC gc = new GC(image);
      this.print(gc);
      gc.dispose();
      return image;
   }
}
