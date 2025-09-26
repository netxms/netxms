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
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.modules.charts.api.ChartColor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.modules.dashboards.views.DrilldownDashboardView;
import org.netxms.nxmc.modules.networkmaps.views.AdHocPredefinedMapView;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Generic chart widget
 */
public class Chart extends Composite
{
   private ChartType type = ChartType.LINE;
   private ChartConfiguration configuration;
   protected ChartColor[] palette = null;
   private List<ChartDciConfig> items = new ArrayList<ChartDciConfig>(ChartConfiguration.MAX_GRAPH_ITEM_COUNT);
   private List<DataSeries> dataSeries = new ArrayList<DataSeries>(ChartConfiguration.MAX_GRAPH_ITEM_COUNT);
   private long drillDownObjectId = 0;
   private ColorCache colorCache;
   private Label title;
   private ChartLegend legend;
   private Composite plotAreaComposite;
   private PlotArea plotArea;
   private boolean mouseDown = false;
   private Set<IDoubleClickListener> doubleClickListeners = new HashSet<IDoubleClickListener>();
   private MenuManager menuManager = null;
   private MouseListener mouseListener = null;
   private View view;

   /**
    * Create empty chart control.
    *
    * @param parent parent control
    * @param style chart control style
    * @param view view that contains this chart
    */
   public Chart(Composite parent, int style, View view)
   {
      this(parent, style, ChartType.LINE, null, view);
   }

   /**
    * Create chart control with given configuration.
    *
    * @param parent parent control
    * @param style chart control style
    * @param type chart type
    * @param configuration chart configuration
    * @param view view that contains this chart (can be null)
    */
   public Chart(Composite parent, int style, ChartType type, ChartConfiguration configuration, View view)
   {
      super(parent, style);

      this.view = view;

      colorCache = new ColorCache(this);

      createDefaultPalette();
      setBackground(ThemeEngine.getBackgroundColor("Chart.Base"));

      mouseListener = new MouseListener() {
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
            fireDoubleClickListeners();
         }
      };
      addMouseListener(mouseListener);

      this.type = type;
      this.configuration = configuration;
      if (configuration != null)
         rebuild();
   }

   /**
    * Get chart type
    *
    * @return chart type
    */
   public ChartType getType()
   {
      return type;
   }

   /**
    * Set chart type
    *
    * @param type new chart type
    */
   public void setType(ChartType type)
   {
      this.type = type;
   }

   /**
    * Check if current chart type has axes.
    *
    * @return true if current chart type has axes
    */
   public boolean hasAxes()
   {
      return type != ChartType.PIE;
   }

   /**
    * Get metrics
    *
    * @return lis of metrics
    */
   protected List<ChartDciConfig> getItems()
   {
      return items;
   }

   /**
    * Get specific item
    *
    * @param index item index
    * @return item or null
    */
   public ChartDciConfig getItem(int index)
   {
      try
      {
         return items.get(index);
      }
      catch(IndexOutOfBoundsException e)
      {
         return null;
      }
   }

   /**
    * Get number of items on chart.
    *
    * @return number of items on chart
    */
   public int getItemCount()
   {
      return items.size();
   }

   /**
    * Get data series
    *
    * @return list of data series
    */
   protected List<DataSeries> getDataSeries()
   {
      return dataSeries;
   }

   /**
    * Get chart's color cache.
    *
    * @return chart's color cache
    */
   protected ColorCache getColorCache()
   {
      return colorCache;
   }

   /**
    * Get current chart configuration
    *
    * @return current chart configuration
    */
   public ChartConfiguration getConfiguration()
   {
      return configuration;
   }

   /**
    * Re-create chart with new configuration.
    *
    * @param configuration new configuration
    */
   public void reconfigure(ChartConfiguration configuration)
   {
      this.configuration = configuration;
      rebuild();
   }

   /**
    * Re-create chart with current configuration.
    */
   public void rebuild()
   {
      if (plotAreaComposite != null)
         plotAreaComposite.dispose();
      if (title != null)
         title.dispose();
      if (legend != null)
         legend.dispose();

      if (configuration != null)
      {
         GridLayout layout = new GridLayout();
         layout.numColumns = isLegendOnSide() ? 2 : 1;
         layout.marginWidth = 5;
         layout.marginHeight = 5;
         setLayout(layout);

         if (configuration.isTitleVisible())
         {
            createTitle();
         }
         else
         {
            title = null;
         }

         if (configuration.isLegendVisible() && ((configuration.getLegendPosition() == ChartConfiguration.POSITION_LEFT) || (configuration.getLegendPosition() == ChartConfiguration.POSITION_TOP)))
            createLegend();
         createPlotArea();
         if (configuration.isLegendVisible() && ((configuration.getLegendPosition() == ChartConfiguration.POSITION_RIGHT) || (configuration.getLegendPosition() == ChartConfiguration.POSITION_BOTTOM)))
            createLegend();
      }
      else
      {
         plotAreaComposite = null;
         plotArea = null;
         title = null;
         legend = null;
      }

      layout(true, true);
   }

   /**
    * Create chart title widget
    *
    * @param insertBefore insert before given control
    */
   private void createTitle()
   {
      title = new Label(this, SWT.NONE);
      title.setBackground(getBackground());
      title.setText(configuration.getTitle().replace("&", "&&"));
      title.setFont(JFaceResources.getBannerFont());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.horizontalSpan = isLegendOnSide() ? 2 : 1;
      title.setLayoutData(gd);
      if (menuManager != null)
         title.setMenu(menuManager.createContextMenu(title));
   }

   /**
    * Create legend widget
    */
   private void createLegend()
   {
      legend = new ChartLegend(this, ThemeEngine.getForegroundColor("Chart.Base"), isLegendOnSide());
      GridData gd = new GridData();
      if (isLegendOnSide())
      {
         gd.horizontalAlignment = SWT.CENTER;
         gd.verticalAlignment = SWT.TOP;
         gd.grabExcessVerticalSpace = true;
      }
      else
      {
         gd.horizontalAlignment = SWT.LEFT;
         gd.verticalAlignment = SWT.CENTER;
         gd.grabExcessHorizontalSpace = true;
      }
      legend.setLayoutData(gd);
      if (menuManager != null)
      {
         Menu menu = menuManager.createContextMenu(legend);
         legend.setMenu(menu);
         for(Control c : legend.getChildren())
            c.setMenu(menu);
      }
   }

   /**
    * Check if legend is visible and is located on left or right size
    *
    * @return true if legend is visible and is located on left or right size
    */
   private boolean isLegendOnSide()
   {
      int legendPosition = configuration.getLegendPosition();
      return configuration.isLegendVisible() && ((legendPosition == ChartConfiguration.POSITION_LEFT) || (legendPosition == ChartConfiguration.POSITION_RIGHT));
   }

   /**
    * Create plot area
    */
   private void createPlotArea()
   {
      switch(type)
      {
         case BAR:
            plotAreaComposite = new BarChart(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case BAR_GAUGE:
            plotAreaComposite = new BarGauge(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case CIRCULAR_GAUGE:
            plotAreaComposite = new CircularGauge(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case DIAL_GAUGE:
            plotAreaComposite = new DialGauge(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case LINE:
            plotAreaComposite = new LineChart(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case PIE:
            plotAreaComposite = new PieChart(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         case TEXT_GAUGE:
            plotAreaComposite = new TextGauge(this);
            plotArea = (PlotArea)plotAreaComposite;
            break;
         default:
            plotAreaComposite = new Composite(this, SWT.NONE);
            plotAreaComposite.setBackground(getBackground());
            plotArea = null;
      }
      plotAreaComposite.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      if (menuManager != null)
      {
         Menu menu = menuManager.createContextMenu(plotAreaComposite);
         plotAreaComposite.setMenu(menu);
         for(Control c : plotAreaComposite.getChildren())
            c.setMenu(menu);
      }
      plotAreaComposite.addMouseListener(mouseListener);
   }

   /**
    * Set palette.
    * 
    * @param colors colors for series or null to set default
    */
   public void setPalette(ChartColor[] colors)
   {
      if (colors != null)
         palette = colors;
      else
         createDefaultPalette();
   }

   /**
    * Get single palette element.
    * 
    * @param index element index
    */
   public ChartColor getPaletteEntry(int index)
   {
      try
      {
         return palette[index];
      }
      catch(ArrayIndexOutOfBoundsException e)
      {
         return null;
      }
   }

   /**
    * Set single palette element.
    * 
    * @param index element index
    * @param color color for series
    */
   public void setPaletteEntry(int index, ChartColor color)
   {
      try
      {
         palette[index] = color;
      }
      catch(ArrayIndexOutOfBoundsException e)
      {
      }
   }

   /**
    * Create default palette from preferences
    */
   protected void createDefaultPalette()
   {
      palette = new ChartColor[ChartConfiguration.MAX_GRAPH_ITEM_COUNT];
      for(int i = 0; i < ChartConfiguration.MAX_GRAPH_ITEM_COUNT; i++)
      {
         palette[i] = ChartColor.getDefaultColor(i);
      }
   }

   /**
    * Add metric
    * 
    * @param parameter DCI information
    * @param value parameter's initial value
    * @return parameter's index (0 .. MAX_CHART_ITEMS-1)
    */
   public int addParameter(ChartDciConfig metric)
   {
      if (items.size() >= ChartConfiguration.MAX_GRAPH_ITEM_COUNT)
         return -1;

      items.add(metric);
      dataSeries.add(new DataSeries());
      return items.size() - 1;
   }

   /**
    * Remove all metrics from chart
    */
   public void removeAllParameters()
   {
      items.clear();
      dataSeries.clear();
   }

   /**
    * Remove data metrics from chart
    */
   public void clearParameters()
   {
      for(int i = 0; i < dataSeries.size(); i++)
         dataSeries.set(i, new DataSeries(0));
   }

   /**
    * Get threshold configuraiton for item
    * 
    * @param i time index
    * @return
    */
   public Threshold[] getThreshold(int i)
   {
      return dataSeries.size() > i ? dataSeries.get(i).getThresholds() : null;
   }

   /**
    * Update values for parameter
    * 
    * @param index parameter's index (0 .. MAX_CHART_ITEMS-1)
    * @param value parameter's value
    * @param updateChart if true, chart will be updated (repainted)
    */
   public void updateParameter(int index, DataSeries values, boolean updateChart)
   {
      if (index >= ChartConfiguration.MAX_GRAPH_ITEM_COUNT)
         return;
      
      dataSeries.set(index, new DataSeries(values));
      if (updateChart)
         refresh();
   }

   /**
    * Update value for parameter
    * 
    * @param index parameter's index (0 .. MAX_CHART_ITEMS-1)
    * @param value parameter's value
    * @param updateChart if true, chart will be updated (repainted)
    */
   public void updateParameter(int index, double value, boolean updateChart)
   {
      if (index >= ChartConfiguration.MAX_GRAPH_ITEM_COUNT)
         return;
      
      dataSeries.set(index, new DataSeries(value));
      if (updateChart)
         refresh();
   }

   /**
    * Set time range for chart.
    * 
    * @param from start time
    * @param to end time
    */
   public void setTimeRange(final Date from, final Date to)
   {
      if (plotArea instanceof LineChart)
         ((LineChart)plotArea).setTimeRange(from, to);
   }

   /**
    * Adjust X axis to fit all data
    * 
    * @param repaint if true, chart will be repainted after change
    */
   public void adjustXAxis(boolean repaint)
   {
      if (plotArea instanceof LineChart)
         ((LineChart)plotArea).adjustXAxis(repaint);
   }

   /**
    * Adjust Y axis to fit all data
    * 
    * @param repaint if true, chart will be repainted after change
    */
   public void adjustYAxis(boolean repaint)
   {
      if (plotArea instanceof LineChart)
         ((LineChart)plotArea).adjustYAxis(repaint);
   }

   /**
    * Zoom in
    */
   public void zoomIn()
   {
      if (plotArea instanceof LineChart)
         ((LineChart)plotArea).zoomIn();
   }

   /**
    * Zoom out
    */
   public void zoomOut()
   {
      if (plotArea instanceof LineChart)
         ((LineChart)plotArea).zoomOut();
   }

   /**
    * Refresh (repaint) chart using current data and settings
    */
   public void refresh()
   {
      if (configuration.isLegendVisible() && configuration.isExtendedLegend())
         legend.refresh();
      if (plotArea != null)
         plotArea.refresh();
   }

   /**
    * Set menu manager for context menu.
    *
    * @param menuManager menu manager for context menu
    */
   public void setMenuManager(MenuManager menuManager)
   {
      this.menuManager = menuManager;
      if (menuManager != null)
      {
         if (title != null)
         {
            title.setMenu(menuManager.createContextMenu(title));
         }
         if (legend != null)
         {
            Menu menu = menuManager.createContextMenu(legend);
            legend.setMenu(menu);
            for(Control c : legend.getChildren())
               c.setMenu(menu);
         }
         if (plotAreaComposite != null)
         {
            Menu menu = menuManager.createContextMenu(plotAreaComposite);
            plotAreaComposite.setMenu(menu);
            for(Control c : plotAreaComposite.getChildren())
               c.setMenu(menu);
         }
      }
   }

   /**
    * Get ID of drill-down object for this gauge (dashboard or network map)
    */
   public long getDrillDownObjectId()
   {
      return drillDownObjectId;
   }

   /**
    * Set ID of drill-down object for this gauge (dashboard or network map)
    * 
    * @param objectId ID of drill-down object or 0 to disable drill-down functionality
    */
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
      if (view == null)
         return;

      AbstractObject object = Registry.getSession().findObjectById(drillDownObjectId);
      if (object == null)
         return;

      if (object instanceof Dashboard)
      {
         Dashboard dashboard = (Dashboard)object;
         AbstractObject dashboardContext = (view instanceof AbstractDashboardView) ? ((AbstractDashboardView)view).getDashboardContext() : null;
         long dashboardContextId = (dashboardContext != null) ? dashboardContext.getObjectId() : 0;
         long contextObjectId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
         view.openView(new DrilldownDashboardView(dashboard, dashboardContextId, contextObjectId));
      }
      else if (object instanceof NetworkMap)
      {
         NetworkMap map = (NetworkMap)object;
         long contextObjectId = (view instanceof ObjectView) ? ((ObjectView)view).getObjectId() : 0;
         view.openView(new AdHocPredefinedMapView(contextObjectId, map));
      }
   }

   /**
    * Add double click listener. Listener will be called on double click on any chart element - plot area, legend, or title.
    *
    * @param listener double click listener
    */
   public void addDoubleClickListener(IDoubleClickListener listener)
   {
      doubleClickListeners.add(listener);
   }

   /**
    * Remove previously added double click listener.
    *
    * @param listener double click listener
    */
   public void removeDoubleClickListener(IDoubleClickListener listener)
   {
      doubleClickListeners.add(listener);
   }

   /**
    * Fire registered double click listeners
    */
   protected void fireDoubleClickListeners()
   {
      for(IDoubleClickListener l : doubleClickListeners)
         l.doubleClick(null);
   }

   /**
    * Take snapshot of this chart.
    *
    * @return image containing snapshot of this chart
    */
   private Image takeSnapshot()
   {
      Rectangle rect = getClientArea();
      Image image = new Image(getDisplay(), rect.width, rect.height);
      GC gc = new GC(image);
      this.print(gc);
      gc.dispose();
      return image;
   }

   /**
    * Copy to clipboard as an image
    */
   public void copyToClipboard()
   {
      Image image = takeSnapshot();
      WidgetHelper.copyToClipboard(image);
      image.dispose();
   }

   /**
    * Save as image file
    * 
    * @param parentShellparent shell for file save dialog
    */
   public void saveAsImage(Shell parentShell)
   {
      Image image = takeSnapshot();
      WidgetHelper.saveImageToFile(null, "graph.png", image);
      image.dispose();
   }

   /**
    * Returns true if chart has extended legend.
    *
    * @return true if chart has extended legend
    */
   public boolean hasExtendedLegend()
   {
      return configuration.isLegendVisible() && configuration.isExtendedLegend();
   }
}
