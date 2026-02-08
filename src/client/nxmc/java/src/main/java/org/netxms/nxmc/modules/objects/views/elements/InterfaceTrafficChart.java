/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.elements;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.TimePeriod;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.InterfaceTrafficDcis;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView;
import org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView.ChartActionType;
import org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView.HistoricalChartOwner;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * DCI last values
 */
public class InterfaceTrafficChart extends OverviewPageElement implements HistoricalChartOwner
{
   private static Logger logger = LoggerFactory.getLogger(InterfaceTrafficChart.class);

   private final I18n i18n = LocalizationHelper.getI18n(InterfaceTrafficChart.class);

   private Chart chart;
   private InterfaceTrafficDcis items = null;
   ChartConfiguration chartConfiguration = new ChartConfiguration();
   private ViewRefreshController refreshController;
   private Composite content;
   private Composite labelControl;
   private Label label;
   private Action actionAdjustX;
   private Action actionAdjustY;
   private Action actionAdjustBoth;
   private Action[] presetActions;
   private boolean utilization;
   private int itemBase;

   /**
    * @param parent
    * @param anchor
    * @param objectTab
    */
   public InterfaceTrafficChart(Composite parent, OverviewPageElement anchor, ObjectView objectView, boolean utilization)
   {
      super(parent, anchor, objectView);
      refreshController = new ViewRefreshController(objectView, -1, () -> refresh());
      this.utilization = utilization;
      itemBase = utilization ? 2 : 0;
      createActions();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return utilization ? i18n.tr("Utilization") : i18n.tr("Traffic");
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createClientArea(Composite parent)
   {      
      content = new Composite(parent, SWT.NONE);
      content.addControlListener(new ControlListener() {
         @Override
         public void controlResized(ControlEvent e)
         {
            chart.setSize(content.getSize());
            labelControl.setSize(content.getSize());
         }
         
         @Override
         public void controlMoved(ControlEvent e)
         {
         }
      });

      final Date from = new Date(System.currentTimeMillis() - chartConfiguration.getTimeRangeMillis());
      final Date to = new Date(System.currentTimeMillis());

      chartConfiguration.setZoomEnabled(false);
      chartConfiguration.setTitleVisible(false);
      chartConfiguration.setLegendVisible(true);
      chartConfiguration.setExtendedLegend(true);
      chartConfiguration.setUseMultipliers(true);
      chartConfiguration.setAutoScale(true);
      chartConfiguration.setTimePeriod(new TimePeriod()); 
      chart = new Chart(content, SWT.NONE, ChartType.LINE, chartConfiguration, null);
      chart.setTimeRange(from, to);
      chart.addDoubleClickListener((e) -> openHistoryGraph());

      chart.addDisposeListener((e) -> {
            refreshController.dispose();
      });
      createChartContextMenu();

      labelControl = new Composite(content, SWT.NONE);
      labelControl.setLayout(new GridLayout());
      label = new Label(labelControl, SWT.CENTER);
      label.setText("Loading...");
      GridData gd = new GridData(SWT.CENTER, SWT.CENTER, true, true);
      label.setLayoutData(gd);
      
      refreshController.setInterval(30);
      return content;
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionAdjustX = HistoricalGraphView.createAction(ChartActionType.ADJUST_X, this);
      actionAdjustY = HistoricalGraphView.createAction(ChartActionType.ADJUST_Y, this);
      actionAdjustBoth = HistoricalGraphView.createAction(ChartActionType.ADJUST_BOTH, this);

      presetActions = HistoricalGraphView.createPresetActions(new HistoricalGraphView.PresetHandler() {
         @Override
         public void onPresetSelected(TimeUnit unit, int range)
         {
            chartConfiguration.setTimePeriod(new TimePeriod(TimeFrameType.BACK_FROM_NOW, range, unit, null, null));
            refresh();
         }
      });
   }

   /**
    * Create chart's context menu
    */
   private void createChartContextMenu()
   {
      final MenuManager manager = new MenuManager();
      manager.setRemoveAllWhenShown(true);
      manager.addMenuListener((m) -> fillContextMenu(m));
      chart.setMenuManager(manager);
   }

   /**
    * Fill context menu
    * 
    * @param manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      MenuManager presets = new MenuManager("&Presets");
      for(int i = 0; i < presetActions.length; i++)
         presets.add(presetActions[i]);

      manager.add(presets);
      manager.add(new Separator());
      manager.add(actionAdjustBoth);
      manager.add(actionAdjustX);
      manager.add(actionAdjustY);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getHeightHint()
    */
   @Override
   protected int getHeightHint()
   {
      return 300;
   }

   /**
    * Open history graph of dci
    */
   private void openHistoryGraph()
   {
      List<ChartDciConfig> graphItems = new ArrayList<ChartDciConfig>(items.getDciList().length);
      for(int i = 0; i < chart.getItemCount(); i++)
         graphItems.add(new ChartDciConfig(chart.getItem(i)));

      getObjectView().openView(new HistoricalGraphView(getObject(), graphItems, chart.getConfiguration(), 0));
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#onObjectChange()
    */
   @Override
   protected void onObjectChange()
   {
      items = null;
      chart.removeAllParameters();
      refresh();
   }
   
   /**
    * Refresh element
    */
   private void refresh()
   {
      final NXCSession session = Registry.getSession();
      final long nodeId = ((Interface)getObject()).getParentNode().getObjectId();
      Job job = new Job(i18n.tr("Read last DCI values"), getObjectView()) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {               
               final Date from = new Date(System.currentTimeMillis() - chartConfiguration.getTimeRangeMillis());
               final Date to = new Date(System.currentTimeMillis());
               final InterfaceTrafficDcis perfData = session.getInterfaceTrafficDcis(getObject().getObjectId());
               final DataSeries[] data = new DataSeries[2];
               for(int i = 0; i < data.length; i++)
               {
                  Long currentDci = perfData.getDciList()[itemBase + i];
                  if (currentDci == 0)
                  {
                     data[i] = null;
                     continue;
                  }
                  data[i] = session.getCollectedData(nodeId, currentDci, from, to, 0, HistoricalDataType.PROCESSED);
               }
               
               runInUIThread(() -> {
                  if (!chart.isDisposed())
                  {
                     if (items == null)
                     {                           
                        items = perfData;
                        createChart(nodeId);
                     }
                     else
                     {
                        items = perfData;
                     }

                     chart.setTimeRange(from, to);

                     if (data[0] != null)
                     {
                        chart.updateParameter(0, data[0], false);
                        if (data[1] != null)
                           chart.updateParameter(1, data[1], false);
                     }
                     else
                     {
                        if (data[1] != null)
                           chart.updateParameter(0, data[1], false);
                     }

                     chart.refresh();
                  }
               });
            }
            catch(Exception e)
            {
               logger.error("Exception in interface trafic overview element", e);
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot read last DCI values");
         }
      };
      job.setUser(false);
      job.setSystem(true);
      job.start();
   }

   private void createChart(Long nodeId)
   {
      if (items.getDciList()[itemBase] == 0 && items.getDciList()[itemBase + 1] == 0)
      {
         label.setText("No data");
         labelControl.moveAbove(null);
         return;
      }

      chart.moveAbove(null);

      if (items.getDciList()[itemBase] != 0)
      {
         ChartDciConfig item = new ChartDciConfig();
         item.nodeId = nodeId;
         item.dciId = items.getDciList()[itemBase];
         item.name = "RX";
         item.lineChartType = ChartDciConfig.DEFAULT;
         item.invertValues = false;
         item.showThresholds = true;
         chart.addParameter(item);
      }

      if (items.getDciList()[itemBase + 1] != 0)
      {
         ChartDciConfig item = new ChartDciConfig();
         item.nodeId = nodeId;
         item.dciId = items.getDciList()[itemBase + 1];
         item.name = "TX";
         item.lineChartType = ChartDciConfig.DEFAULT;
         item.invertValues = (items.getDciList()[itemBase + 1] != 0);
         item.showThresholds = true;
         chart.addParameter(item);
      }
      chart.rebuild();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return object instanceof Interface;
   }

   /**
    * @see org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView.HistoricalChartOwner#getChart()
    */
   @Override
   public Chart getChart()
   {
      return chart;
   }
}
