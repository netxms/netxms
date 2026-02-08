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
package org.netxms.nxmc.modules.dashboards.views;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DashboardBase;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.widgets.DashboardControl;
import org.netxms.nxmc.modules.dashboards.widgets.ElementWidget;
import org.netxms.nxmc.modules.dashboards.widgets.LineChartElement;
import org.netxms.nxmc.modules.dashboards.widgets.LineChartElement.DataCacheElement;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for different dashboard views
 */
public abstract class AbstractDashboardView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(AbstractDashboardView.class);  

   protected DashboardControl dbc;

   private Composite viewArea;
   private ScrolledComposite scroller;
   private boolean narrowScreenMode = false;
   private Action actionExportValues;
   private Action actionSaveAsImage;
   private Action actionNarrowScreenMode;

   /**
    * Create view.
    *
    * @param name view name
    * @param image view image
    * @param id view ID
    */
   public AbstractDashboardView(String name, ImageDescriptor image, String id)
   {
      super(name, image, id, false); 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      viewArea = parent;

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      viewArea.setLayout(layout);

      createActions();
   }

   /**
    * Create actions
    */
   protected void createActions()
   {
      actionExportValues = new Action(i18n.tr("E&xport line chart values"), ResourceManager.getImageDescriptor("icons/export-data.png")) {
         @Override
         public void run()
         {
            exportLineChartValues();
         }
      };
      actionExportValues.setActionDefinitionId("org.netxms.ui.eclipse.dashboard.commands.export_line_chart_values"); 
      addKeyBinding("M1+F3", actionExportValues);

      actionSaveAsImage = new Action(i18n.tr("Save as &image"), SharedIcons.SAVE_AS_IMAGE) {
         @Override
         public void run()
         {
            saveAsImage();
         }
      };
      actionSaveAsImage.setActionDefinitionId("org.netxms.ui.eclipse.dashboard.commands.save_as_image"); 
      addKeyBinding("M1+I", actionSaveAsImage);

      actionNarrowScreenMode = new Action(i18n.tr("&Narrow screen mode"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            narrowScreenMode = actionNarrowScreenMode.isChecked();
            rebuildCurrentDashboard();
         }
      };
      actionNarrowScreenMode.setChecked(narrowScreenMode);
      addKeyBinding("M1+M3+N", actionNarrowScreenMode);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExportValues);
      manager.add(actionSaveAsImage);
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExportValues);
      manager.add(actionSaveAsImage);
      manager.add(new Separator());
      manager.add(actionNarrowScreenMode);
      super.fillLocalMenu(manager);
   }

   /**
    * Export all line chart values as CSV
    */
   private void exportLineChartValues()
   {
      FileDialog fd = new FileDialog(getWindow().getShell(), SWT.SAVE);
      fd.setFileName(getObjectName() + ".csv");
      final String fileName = fd.open();
      if (fileName == null)
         return;
      
      final List<DataCacheElement> data = new ArrayList<DataCacheElement>();
      for(ElementWidget w : dbc.getElementWidgets())
      {
         if (!(w instanceof LineChartElement))
            continue;

         for(DataCacheElement d : ((LineChartElement)w).getDataCache())
         {
            data.add(d);
         }
      }

      final DateFormat dfDate = DateFormatFactory.getDateFormat();
      final DateFormat dfTime = DateFormatFactory.getTimeFormat();
      final DateFormat dfDateTime = DateFormatFactory.getDateTimeFormat();
      
      new Job(i18n.tr("Export line chart data"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            boolean doInterpolation = session.getPublicServerVariableAsBoolean("Client.DashboardDataExport.EnableInterpolation"); //$NON-NLS-1$
            
            // Build combined time series
            // Time stamps in series are reversed - latest value first
            List<Date> combinedTimeSeries = new ArrayList<Date>();
            for(DataCacheElement d : data)
            {
               for(DciDataRow r : d.data.getValues())
               {
                  int i;
                  for(i = 0; i < combinedTimeSeries.size(); i++)
                  {
                     if (combinedTimeSeries.get(i).getTime() == r.getTimestamp().getTime())
                        break;                 
                     if (combinedTimeSeries.get(i).getTime() < r.getTimestamp().getTime())
                     {
                        combinedTimeSeries.add(i, r.getTimestamp());
                        break;
                     }
                  }
                  if (i == combinedTimeSeries.size())
                     combinedTimeSeries.add(r.getTimestamp());
               }
            }

            List<Double[]> combinedData = new ArrayList<Double[]>(data.size());

            // insert missing values
            for(DataCacheElement d : data)
            {
               Double[] ySeries = new Double[combinedTimeSeries.size()];
               int combinedIndex = 0;
               double lastValue = 0;
               long lastTimestamp = 0;
               DciDataRow[] values = d.data.getValues();
               for(int i = 0; i < values.length; i++)
               {
                  Date currentTimestamp = values[i].getTimestamp();
                  double currentValue = values[i].getValueAsDouble();
                  long currentCombinedTimestamp = combinedTimeSeries.get(combinedIndex).getTime();
                  while(currentCombinedTimestamp > currentTimestamp.getTime())
                  {
                     if ((lastTimestamp != 0) && doInterpolation)
                     {
                        // do linear interpolation for missed value
                        ySeries[combinedIndex] = lastValue + (currentValue - lastValue) * ((double)(lastTimestamp - currentCombinedTimestamp) / (double)(lastTimestamp - currentTimestamp.getTime()));
                     }
                     else
                     {
                        ySeries[combinedIndex] = null;
                     }
                     combinedIndex++;
                     currentCombinedTimestamp = combinedTimeSeries.get(combinedIndex).getTime();
                  }
                  ySeries[combinedIndex++] = currentValue;
                  lastTimestamp = currentTimestamp.getTime();
                  lastValue = currentValue;
               }
               combinedData.add(ySeries);
            }

            BufferedWriter writer = new BufferedWriter(new FileWriter(fileName));
            try
            {
               writer.write("# " + getObjectName() + " " + dfDateTime.format(new Date())); //$NON-NLS-1$ //$NON-NLS-2$
               writer.newLine();
               writer.write("DATE,TIME"); //$NON-NLS-1$
               for(DataCacheElement d : data)
               {
                  writer.write(',');
                  writer.write(d.name);
               }
               writer.newLine();
               
               for(int i = combinedTimeSeries.size() - 1; i >= 0; i--)
               {
                  Date d = combinedTimeSeries.get(i);
                  writer.write(dfDate.format(d));
                  writer.write(',');
                  writer.write(dfTime.format(d));
                  for(Double[] values : combinedData)
                  {
                     writer.write(',');
                     if (values[i] != null)
                     {
                        double v = values[i];
                        if (Math.abs(v) > 0.001)
                           writer.write(String.format("%.3f", v)); //$NON-NLS-1$
                        else
                           writer.write(Double.toString(v));
                     }
                  }  
                  writer.newLine();
               }
            }
            finally
            {
               writer.close();
            }
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot export line chart data");
         }
      }.start();
   }

   /**
    * Take snapshot of the dashboard
    *
    * @return snapshot of the dashboard
    */
   public void saveAsImage()
   {
      Rectangle rect = dbc.getClientArea();
      Image image = new Image(dbc.getDisplay(), rect.width, rect.height);
      GC gc = new GC(image);
      dbc.print(gc);
      gc.dispose();

      FileDialog fd = new FileDialog(getWindow().getShell(), SWT.SAVE);
      fd.setText("Save dashboard as image");
      String[] filterExtensions = { "*.*" }; //$NON-NLS-1$
      fd.setFilterExtensions(filterExtensions);
      String[] filterNames = { ".png" };
      fd.setFilterNames(filterNames);
      String dateTime = new SimpleDateFormat("yyyyMMdd-HHmmss").format(new Date());
      fd.setFileName(getObjectName() + "_" + dateTime + ".png");
      final String selected = fd.open();
      if (selected == null)
         return;

      ImageLoader saver = new ImageLoader();
      saver.data = new ImageData[] { image.getImageData() };
      saver.save(selected, SWT.IMAGE_PNG);

      image.dispose();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectUpdate(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectUpdate(AbstractObject object)
   {
      super.onObjectUpdate(object);
      onObjectChange(object);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      rebuildCurrentDashboard();
   }

   /**
    * Request complete dashboard layout
    */
   public void requestDashboardLayout()
   {
      if (scroller != null)
         updateScroller();
      else
         viewArea.layout(true, true);
   }

   /**
    * Rebuild current dashboard
    */
   protected abstract void rebuildCurrentDashboard();

   /**
    * Rebuild dashboard
    */
   protected void rebuildDashboard(DashboardBase dashboard, AbstractObject dashboardContext)
   {
      if (dbc != null)
         dbc.dispose();

      if ((scroller != null) && !dashboard.isScrollable() && !narrowScreenMode)
      {
         scroller.dispose();
         scroller = null;
      }
      else if ((scroller == null) && (dashboard.isScrollable() || narrowScreenMode))
      {
         scroller = new ScrolledComposite(viewArea, SWT.V_SCROLL);
         scroller.setExpandHorizontal(true);
         scroller.setExpandVertical(true);
         WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
         scroller.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
         viewArea.layout(true, true);
      }

      dbc = new DashboardControl((dashboard.isScrollable() || narrowScreenMode) ? scroller : viewArea, SWT.NONE, dashboard, dashboardContext, this, false, narrowScreenMode);
      if (dashboard.isScrollable() || narrowScreenMode)
      {
         scroller.setContent(dbc);
         scroller.addControlListener(new ControlAdapter() {
            public void controlResized(ControlEvent e)
            {
               updateScroller();
            }
         });
         updateScroller();
      }
      else
      {
         dbc.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
         viewArea.layout(true, true);
      }
   }

   /**
    * Update scroller
    */
   private void updateScroller()
   {
      dbc.layout(true, true);
      Rectangle r = scroller.getClientArea();
      Point s = dbc.computeSize(r.width, SWT.DEFAULT);
      scroller.setMinSize(s);
   }

   /**
    * Get dashboard context.
    *
    * @return dashboard context
    */
   public AbstractObject getDashboardContext()
   {
      return dbc.getContext();
   }
}
