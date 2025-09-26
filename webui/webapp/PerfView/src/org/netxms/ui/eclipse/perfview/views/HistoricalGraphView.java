/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.perfview.views;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.TimePeriod;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartConfigurationChangeListener;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.GraphDefinition;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.charts.api.ChartType;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.compatibility.GraphItem;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.dialogs.SaveGraphDlg;
import org.netxms.ui.eclipse.perfview.propertypages.DataSources;
import org.netxms.ui.eclipse.perfview.propertypages.General;
import org.netxms.ui.eclipse.perfview.propertypages.Graph;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.ViewRefreshController;

/**
 * History graph view
 */
public class HistoricalGraphView extends ViewPart implements ChartConfigurationChangeListener
{
   public static final String ID = "org.netxms.ui.eclipse.perfview.views.HistoryGraph"; //$NON-NLS-1$
   public static final String PREDEFINED_GRAPH_SUBID = "org.netxms.ui.eclipse.charts.predefinedGraph"; //$NON-NLS-1$

   private static final TimeUnit[] presetUnits = { TimeUnit.MINUTE, TimeUnit.MINUTE, TimeUnit.HOUR, TimeUnit.HOUR, TimeUnit.HOUR,
         TimeUnit.HOUR, TimeUnit.DAY, TimeUnit.DAY, TimeUnit.DAY, TimeUnit.DAY, TimeUnit.DAY, TimeUnit.DAY };
   private static final int[] presetRanges = { 10, 30, 1, 2, 4, 12, 1, 2, 5, 7, 31, 365 };
   private static final String[] presetNames = 
      { Messages.get().HistoricalGraphView_Preset10min, Messages.get().HistoricalGraphView_Preset30min, Messages.get().HistoricalGraphView_Preset1hour, Messages.get().HistoricalGraphView_Preset2hours, Messages.get().HistoricalGraphView_Preset4hours, Messages.get().HistoricalGraphView_Preset12hours, Messages.get().HistoricalGraphView_Preset1day,
        Messages.get().HistoricalGraphView_Preset2days, Messages.get().HistoricalGraphView_Preset5days, Messages.get().HistoricalGraphView_PresetWeek, Messages.get().HistoricalGraphView_PresetMonth, Messages.get().HistoricalGraphView_PresetYear };

   private NXCSession session;
   private Chart chart = null;
   private boolean updateInProgress = false;
   private ViewRefreshController refreshController;
   private Composite chartParent = null;
   private GraphDefinition configuration = new GraphDefinition();
   private boolean multipleSourceNodes = false;

   private Action actionRefresh;
   private Action actionAutoRefresh;
   private Action actionZoomIn;
   private Action actionZoomOut;
   private Action actionAdjustX;
   private Action actionAdjustY;
   private Action actionAdjustBoth;
   private Action actionLogScale;
   private Action actionStacked;
   private Action actionAreaChart;
   private Action actionTranslucent;
   private Action actionShowLegend;
   private Action actionExtendedLegend;
   private Action actionLegendLeft;
   private Action actionLegendRight;
   private Action actionLegendTop;
   private Action actionLegendBottom;
   private Action actionProperties;
   private Action actionSave;
   private Action actionSaveAs;
   private Action actionSaveAsTemplate;
   private Action[] presetActions;
   private Action actionCopyImage;
   private Action actionSaveAsImage;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);

      refreshController = new ViewRefreshController(this, -1, new Runnable() {
         @Override
         public void run()
         {
            if (((Widget)chart).isDisposed())
               return;

            updateChart();
         }
      });

      session = ConsoleSharedData.getSession();

      // Extract DCI ids from view id
      // (first field will be unique view id, so we skip it)
      String id = site.getSecondaryId();
      String[] fields = id.split("&"); //$NON-NLS-1$
      if (!fields[0].equals(PREDEFINED_GRAPH_SUBID))
      {
         configuration.setTimePeriod(new TimePeriod(TimeFrameType.BACK_FROM_NOW, session.getClientConfigurationHintAsInt("DefaultLineChartPeriod", 60), TimeUnit.MINUTE, null, null));

         List<ChartDciConfig> items = new ArrayList<ChartDciConfig>();
         for(int i = 1; i < fields.length; i++)
         {
            String[] subfields = fields[i].split("\\@"); //$NON-NLS-1$
            if (subfields.length == 0)
               continue;
            if (Integer.parseInt(subfields[0]) == ChartDciConfig.ITEM)
            {
               try
               {
                  ChartDciConfig dci = new ChartDciConfig();
                  dci.type = Integer.parseInt(subfields[0]);
                  dci.nodeId = Long.parseLong(subfields[1], 10);
                  dci.dciId = Long.parseLong(subfields[2], 10);
                  dci.name = URLDecoder.decode(subfields[3], "UTF-8"); //$NON-NLS-1$
                  dci.dciDescription = URLDecoder.decode(subfields[3], "UTF-8"); //$NON-NLS-1$
                  dci.dciName = URLDecoder.decode(subfields[4], "UTF-8"); //$NON-NLS-1$

                  // Extra fields
                  if (subfields.length >= 6)
                     dci.useRawValues = Boolean.parseBoolean(subfields[5]); 
                  if (subfields.length >= 7)
                     dci.invertValues = Boolean.parseBoolean(subfields[6]);
                  if (subfields.length >= 8)
                     dci.lineChartType = Integer.parseInt(subfields[7]);
                  if (subfields.length >= 9)
                  {
                     Integer color = Integer.parseInt(subfields[8]);
                     if (color == -1)
                        dci.color = ChartDciConfig.UNSET_COLOR;
                     else
                        dci.color = "0x" + Integer.toHexString(color);
                  }
                  if (subfields.length >= 10)
                     configuration.setStacked(Boolean.parseBoolean(subfields[9]));

                  items.add(dci);
               }
               catch(NumberFormatException e)
               {
                  e.printStackTrace();
               }
               catch(UnsupportedEncodingException e)
               {
                  e.printStackTrace();
               }
            }
            else if (Integer.parseInt(subfields[0]) == ChartDciConfig.TABLE)
            {
               try
               {
                  ChartDciConfig dci = new ChartDciConfig();
                  dci.type = Integer.parseInt(subfields[0]);
                  dci.nodeId = Long.parseLong(subfields[1], 10);
                  dci.dciId = Long.parseLong(subfields[2], 10);
                  dci.name = URLDecoder.decode(subfields[3], "UTF-8"); //$NON-NLS-1$
                  dci.dciName = URLDecoder.decode(subfields[3], "UTF-8"); //$NON-NLS-1$
                  dci.instance = URLDecoder.decode(subfields[4], "UTF-8"); //$NON-NLS-1$
                  dci.column = URLDecoder.decode(subfields[5], "UTF-8"); //$NON-NLS-1$
                  
                  items.add(dci);
               }
               catch(NumberFormatException e)
               {
                  e.printStackTrace();
               }
               catch(UnsupportedEncodingException e)
               {
                  e.printStackTrace();
               }
            }
         }

         // Set view title to "host name: dci description" if we have only one DCI
         if (items.size() == 1)
         {
            ChartDciConfig item = items.get(0);
            AbstractObject object = session.findObjectById(item.nodeId);
            if (object != null)
            {
               setPartName(object.getObjectName() + ": " + item.name); //$NON-NLS-1$
            }
         }
         else if (items.size() > 1)
         {
            long nodeId = items.get(0).nodeId;
            for(ChartDciConfig item : items)
               if (item.nodeId != nodeId)
               {
                  nodeId = -1;
                  break;
               }
            if (nodeId != -1)
            {
               // All DCIs from same node, set title to "host name"
               AbstractObject object = session.findObjectById(nodeId);
               if (object != null)
               {
                  setPartName(String.format(Messages.get().HistoricalGraphView_PartName, object.getObjectName()));
               }
            }
         }
         configuration.setTitle(getPartName());
         configuration.setDciList(items.toArray(new ChartDciConfig[items.size()]));
      }
   }

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
    */
   @Override
   public void init(IViewSite site, final IMemento memento) throws PartInitException
   {
      init(site);

      if (memento != null)
      {
         final long id;
         try
         {
            id = Long.parseLong(memento.getTextData()); // to prevent errors on old memento format
            if (id == 0)
               return;
         }
         catch(Exception e)
         {
            e.printStackTrace();
            return;
         }

         ConsoleJob job = new ConsoleJob(Messages.get().HistoricalGraphView_JobName, this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               final GraphDefinition s = session.getPredefinedGraph(id);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     configuration = s;
                     configureGraphFromSettings();
                     configuration.addChangeListener(HistoricalGraphView.this);
                  }
               });
            }

            @Override
            protected String getErrorMessage()
            {
               return null;
            }
         };
         job.setUser(false);
         job.start();
      }
   }

   /**
    * @see org.eclipse.ui.part.ViewPart#saveState(org.eclipse.ui.IMemento)
    */
   @Override
   public void saveState(IMemento memento)
   {
      try
      {
         memento.putTextData(Long.toString(configuration.getId()));
      }
      catch(Exception e)
      {
      }
   }

   /**
    * Initialize this view with predefined graph settings
    * 
    * @param definition graph settings
    */
   public void initPredefinedGraph(GraphDefinition definition)
   {
      configuration = new GraphDefinition(definition);
      configuration.addChangeListener(this);
      configureGraphFromSettings();
   }

   /**
    * Configure graph from graph settings
    */
   private void configureGraphFromSettings()
   {
      if (chart != null)
         chart.dispose();

      setPartName(configuration.getTitle());

      ChartConfiguration chartConfiguration = new ChartConfiguration(configuration);
      chartConfiguration.setZoomEnabled(true);
      chart = new Chart(chartParent, SWT.NONE, ChartType.LINE, chartConfiguration);
      createPopupMenu();

      // Data
      int nodeId = 0;
      for(ChartDciConfig dci : configuration.getDciList())
      {
         nodeId |= dci.nodeId; // Check that all DCI's are from one node
         GraphItem item = new GraphItem(dci);
         if (configuration.isShowHostNames())
            item.setDescription(session.getObjectName(dci.nodeId) + " - " + dci.getLabel());
         chart.addParameter(item);
      }
      updateDciInfo();

      // Check that all DCI's are form one node
      if (chart.getItemCount() > 1)
         multipleSourceNodes = (nodeId != configuration.getDciList()[0].nodeId);

      chart.rebuild();
      chartParent.layout(true, true);
      updateChart();

      // Automatic refresh
      actionAutoRefresh.setChecked(configuration.isAutoRefresh());
      refreshMenuSelection();
      refreshController.setInterval(configuration.isAutoRefresh() ? configuration.getRefreshRate() : -1);
   }
   
   private void updateDciInfo()
   {
      ConsoleJob job = new ConsoleJob("Get DCI info", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  chart.rebuild();
                  chartParent.layout(true, true);
                  updateChart();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      };
      job.setUser(false);
      job.start();
      
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      chartParent = parent;

      createActions();
      contributeToActionBars();

      configureGraphFromSettings();
      configuration.addChangeListener(this);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      MenuManager menuManager = new MenuManager();
      menuManager.setRemoveAllWhenShown(true);
      menuManager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });
      chart.setMenuManager(menuManager);
   }

   /**
    * Get DCI data from server
    */
   private void getDataFromServer()
   {
      final ChartDciConfig[] dciList = configuration.getDciList();
      if (dciList.length == 0)
      {
         updateInProgress = false;
         return;
      }

      // Request data from server
      ConsoleJob job = new ConsoleJob(Messages.get().HistoricalGraphView_JobName, this, Activator.PLUGIN_ID) {
         private ChartDciConfig currentItem;

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(getName(), dciList.length);
            final DataSeries[] data = new DataSeries[dciList.length];
            final Threshold[][] thresholds = new Threshold[dciList.length][];
            for(int i = 0; i < dciList.length; i++)
            {
               currentItem = dciList[i];
               if (currentItem.type == ChartDciConfig.ITEM)
               {
                  data[i] = session.getCollectedData(currentItem.nodeId, currentItem.dciId, configuration.getTimeFrom(),
                        configuration.getTimeTo(), 0, currentItem.useRawValues ? HistoricalDataType.RAW : HistoricalDataType.PROCESSED);
                  thresholds[i] = session.getThresholds(currentItem.nodeId, currentItem.dciId);
               }
               else
               {
                  data[i] = session.getCollectedTableData(currentItem.nodeId, currentItem.dciId, currentItem.instance,
                        currentItem.column, configuration.getTimeFrom(), configuration.getTimeTo(), 0);
                  thresholds[i] = null;
               }
               monitor.worked(1);
            }

            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!((Widget)chart).isDisposed())
                  {
                     chart.setTimeRange(configuration.getTimeFrom(), configuration.getTimeTo());
                     setChartData(data, thresholds);
                     chart.clearErrors();
                  }
                  updateInProgress = false;
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().HistoricalGraphView_JobError, session.getObjectName(currentItem.nodeId), currentItem.name);
         }

         @Override
         protected void jobFailureHandler()
         {
            updateInProgress = false;
            super.jobFailureHandler();
         }

         @Override
         protected IStatus createFailureStatus(final Exception e)
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  chart.addError(getErrorMessage() + " (" + e.getLocalizedMessage() + ")"); //$NON-NLS-1$ //$NON-NLS-2$
               }
            });
            return Status.OK_STATUS;
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      ((Composite)chart).setFocus();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            updateChart();
         }
      };

      actionProperties = new Action(Messages.get().HistoricalGraphView_Properties) {
         @Override
         public void run()
         {
            showGraphPropertyPages(configuration);
            configureGraphFromSettings(); // Always refresh graph (last action was cancel, but before could have been apply actions)
         }
      };

      actionAutoRefresh = new Action(Messages.get().HistoricalGraphView_Autorefresh) {
         @Override
         public void run()
         {
            configuration.setAutoRefresh(!configuration.isAutoRefresh());
            setChecked(configuration.isAutoRefresh());
            refreshController.setInterval(configuration.isAutoRefresh() ? configuration.getRefreshRate() : -1);
         }
      };
      actionAutoRefresh.setChecked(configuration.isAutoRefresh());

      actionLogScale = new Action(Messages.get().HistoricalGraphView_LogScale) {
         @Override
         public void run()
         {
            try
            {
               configuration.setLogScale(!configuration.isLogScale());
               chart.getConfiguration().setLogScale(configuration.isLogScale());
               chart.rebuild();
            }
            catch(IllegalStateException e)
            {
               MessageDialogHelper.openError(getSite().getShell(), Messages.get().HistoricalGraphView_Error,
                     String.format(Messages.get().HistoricalGraphView_LogScaleSwitchError, e.getLocalizedMessage()));
               Activator.logError("Cannot change log scale mode", e); //$NON-NLS-1$
            }
            setChecked(configuration.isLogScale());
         }
      };
      actionLogScale.setChecked(configuration.isLogScale());

      actionZoomIn = new Action(Messages.get().HistoricalGraphView_ZoomIn) {
         @Override
         public void run()
         {
            chart.zoomIn();
         }
      };
      actionZoomIn.setImageDescriptor(SharedIcons.ZOOM_IN);

      actionZoomOut = new Action(Messages.get().HistoricalGraphView_ZoomOut) {
         @Override
         public void run()
         {
            chart.zoomOut();
         }
      };
      actionZoomOut.setImageDescriptor(SharedIcons.ZOOM_OUT);

      final HistoricalChartOwner chartOwner = new HistoricalChartOwner() {
         @Override
         public Chart getChart()
         {
            return HistoricalGraphView.this.chart;
         }
      };
      actionAdjustX = createAction(ChartActionType.ADJUST_X, chartOwner);
      actionAdjustY = createAction(ChartActionType.ADJUST_Y, chartOwner);
      actionAdjustBoth = createAction(ChartActionType.ADJUST_BOTH, chartOwner);

      actionShowLegend = new Action(Messages.get().HistoricalGraphView_ShowLegend) {
         @Override
         public void run()
         {
            configuration.setLegendVisible(actionShowLegend.isChecked());
            chart.getConfiguration().setLegendVisible(configuration.isLegendVisible());
            chart.rebuild();
         }
      };
      actionShowLegend.setChecked(configuration.isLegendVisible());

      actionExtendedLegend = new Action(Messages.get().HistoricalGraphView_ExtendedLegend) {
         @Override
         public void run()
         {
            configuration.setExtendedLegend(actionExtendedLegend.isChecked());
            chart.getConfiguration().setExtendedLegend(configuration.isExtendedLegend());
            chart.rebuild();
         }
      };
      actionExtendedLegend.setChecked(configuration.isExtendedLegend());
      
      actionLegendLeft = new Action(Messages.get().HistoricalGraphView_PlaceOnLeft, Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            configuration.setLegendPosition(ChartConfiguration.POSITION_LEFT);
            chart.getConfiguration().setLegendPosition(configuration.getLegendPosition());
            chart.rebuild();
         }
      };
      actionLegendLeft.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_LEFT);

      actionLegendRight = new Action(Messages.get().HistoricalGraphView_PlaceOnRight, Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            configuration.setLegendPosition(ChartConfiguration.POSITION_RIGHT);
            chart.getConfiguration().setLegendPosition(configuration.getLegendPosition());
            chart.rebuild();
         }
      };
      actionLegendRight.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_RIGHT);

      actionLegendTop = new Action(Messages.get().HistoricalGraphView_PlaceOnTop, Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            configuration.setLegendPosition(ChartConfiguration.POSITION_TOP);
            chart.getConfiguration().setLegendPosition(configuration.getLegendPosition());
            chart.rebuild();
         }
      };
      actionLegendTop.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_TOP);

      actionLegendBottom = new Action(Messages.get().HistoricalGraphView_PlaceOnBottom, Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            configuration.setLegendPosition(ChartConfiguration.POSITION_BOTTOM);
            chart.getConfiguration().setLegendPosition(configuration.getLegendPosition());
            chart.rebuild();
         }
      };
      actionLegendBottom.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_BOTTOM);

      actionSave = new Action("Save", SharedIcons.SAVE) {
         @Override
         public void run()
         {
            if (configuration.getId() == 0)
            {
               String initalName = configuration.getName().compareTo("noname") == 0 ? configuration.getTitle() : configuration.getName();
               saveGraph(initalName, null, false, false, true);
            }
            else
               saveGraph(configuration.getName(), null, false, false, false);
         }
      };     

      actionSaveAs = new Action("Save as...", SharedIcons.SAVE_AS) {
         @Override
         public void run()
         {
            String initalName = configuration.getName().compareTo("noname") == 0 ? configuration.getTitle() : configuration.getName();
            saveGraph(initalName, null, false, false, true);
         }
      };  

      actionSaveAsTemplate = new Action("Save as template") {
         @Override
         public void run()
         {
            String initalName = configuration.getName().compareTo("noname") == 0 ? configuration.getTitle() : configuration.getName();
            saveGraph(initalName, null, false, true, true);
         }
      };

      actionStacked = new Action(Messages.get().HistoricalGraphView_Stacked, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            configuration.setStacked(actionStacked.isChecked());
            configureGraphFromSettings();
         }
      };
      actionStacked.setChecked(configuration.isStacked());

      actionTranslucent = new Action(Messages.get().HistoricalGraphView_Translucent, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            configuration.setTranslucent(actionTranslucent.isChecked());
            configureGraphFromSettings();
         }
      };
      actionTranslucent.setChecked(configuration.isTranslucent());
      
      actionAreaChart = new Action("Area chart", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            configuration.setArea(actionAreaChart.isChecked());
            configureGraphFromSettings();
         }
      };
      actionAreaChart.setChecked(configuration.isArea());

      presetActions = createPresetActions(new PresetHandler() {
         @Override
         public void onPresetSelected(TimeUnit unit, int range)
         {
            configuration.getTimePeriod().setTimeUnit(unit);
            configuration.getTimePeriod().setTimeRange(range);
            updateChart();
         }
      });

      actionCopyImage = new Action(Messages.get().HistoricalGraphView_CopyToClipboard, SharedIcons.COPY) {
         @Override
         public void run()
         {
            chart.copyToClipboard();
         }
      };
      
      actionSaveAsImage = new Action("Save as image...", SharedIcons.SAVE_AS_IMAGE) {
          @Override
          public void run()
          {
            chart.saveAsImage(getSite().getShell());
          }
       };
   }
   
   /**
    * 
    */
   protected void refreshMenuSelection()
   {
      actionAutoRefresh.setChecked(configuration.isAutoRefresh());
      actionLogScale.setChecked(configuration.isLogScale());
      actionShowLegend.setChecked(configuration.isLegendVisible());
      actionExtendedLegend.setChecked(configuration.isExtendedLegend());
      actionStacked.setChecked(configuration.isStacked());
      actionTranslucent.setChecked(configuration.isTranslucent());
      actionAreaChart.setChecked(configuration.isArea());

      actionLegendLeft.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_LEFT);
      actionLegendRight.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_RIGHT);
      actionLegendTop.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_TOP);
      actionLegendBottom.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_BOTTOM);
   }

   /**
    * Fill action bars
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      MenuManager presets = new MenuManager(Messages.get().HistoricalGraphView_Presets);
      for(int i = 0; i < presetActions.length; i++)
         presets.add(presetActions[i]);

      MenuManager legend = new MenuManager(Messages.get().HistoricalGraphView_Legend);
      legend.add(actionShowLegend);
      legend.add(actionExtendedLegend);
      legend.add(new Separator());
      legend.add(actionLegendLeft);
      legend.add(actionLegendRight);
      legend.add(actionLegendTop);
      legend.add(actionLegendBottom);

      manager.add(presets);
      manager.add(new Separator());
      manager.add(actionAdjustBoth);
      manager.add(actionAdjustX);
      manager.add(actionAdjustY);
      manager.add(new Separator());
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(new Separator());
      manager.add(actionAreaChart);
      manager.add(actionStacked);
      manager.add(actionLogScale);
      manager.add(actionTranslucent);
      manager.add(actionAutoRefresh);
      manager.add(legend);
      manager.add(new Separator());
      manager.add(actionRefresh);
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionSaveAsTemplate);
      manager.add(actionProperties);
   }

   /**
    * Fill context menu
    * 
    * @param manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      MenuManager presets = new MenuManager(Messages.get().HistoricalGraphView_Presets);
      for(int i = 0; i < presetActions.length; i++)
         presets.add(presetActions[i]);

      MenuManager legend = new MenuManager(Messages.get().HistoricalGraphView_Legend);
      legend.add(actionShowLegend);
      legend.add(actionExtendedLegend);
      legend.add(new Separator());
      legend.add(actionLegendLeft);
      legend.add(actionLegendRight);
      legend.add(actionLegendTop);
      legend.add(actionLegendBottom);

      manager.add(presets);
      manager.add(new Separator());
      manager.add(actionAdjustBoth);
      manager.add(actionAdjustX);
      manager.add(actionAdjustY);
      manager.add(new Separator());
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(new Separator());
      manager.add(actionAreaChart);
      manager.add(actionStacked);
      manager.add(actionLogScale);
      manager.add(actionTranslucent);
      manager.add(actionAutoRefresh);
      manager.add(legend);
      manager.add(new Separator());
      manager.add(actionRefresh);
      manager.add(new Separator());
      manager.add(actionProperties);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionAdjustBoth);
      manager.add(actionAdjustX);
      manager.add(actionAdjustY);
      manager.add(new Separator());
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionCopyImage);
      manager.add(actionSaveAsImage);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Set chart data
    * 
    * @param data Retrieved DCI data
    * @param thresholds 
    */
   private void setChartData(final DataSeries[] data, Threshold[][] thresholds)
   {
      for(int i = 0; i < data.length; i++)
         chart.updateParameter(i, data[i], false);
      chart.setThresholds(thresholds);
      chart.refresh();
   }

   /**
    * Update chart
    */
   private void updateChart()
   {
      if (updateInProgress)
         return;

      updateInProgress = true;
      if (configuration.getTimePeriod().isBackFromNow())
      {
         configuration.setTimeFrom(new Date(System.currentTimeMillis() - configuration.getTimeRangeMillis()));
         configuration.setTimeTo(new Date(System.currentTimeMillis()));
      }
      getDataFromServer();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      refreshController.dispose();
      super.dispose();
   }

   /**
    * @see org.netxms.client.datacollection.ChartConfigurationChangeListener#onChartConfigurationChange(org.netxms.client.datacollection.ChartConfiguration)
    */
   @Override
   public void onChartConfigurationChange(ChartConfiguration settings)
   {
      if (this.configuration == settings)
         configureGraphFromSettings();
   }

   /**
    * Save this graph as predefined
    */
   private void saveGraph(String graphName, String errorMessage, final boolean canBeOverwritten, final boolean asTemplate, final boolean showNameDialog)
   {
      if (asTemplate && multipleSourceNodes)
      {
         String templateError = "More than one node is used for template creation.\nThis may cause undefined behaviour.";
         errorMessage = errorMessage == null ? templateError : errorMessage+"\n\n" +templateError;
      }

      int result = SaveGraphDlg.OK;
      GraphDefinition template = null;
      final long oldGraphId = configuration.getId();
      if (asTemplate)
      {
         SaveGraphDlg dlg = new SaveGraphDlg(getSite().getShell(), graphName, errorMessage, canBeOverwritten);
         result = dlg.open();
         if (result == Window.CANCEL)
            return;

         template = new GraphDefinition(configuration);
         template.setId(0);
         template.setOwnerId(session.getUserId());
         template.setAccessList(new ArrayList<AccessListElement>(0));
         template.setName(dlg.getName());
         template.setFlags(GraphDefinition.GF_TEMPLATE);
      }
      else
      {
         if (showNameDialog)
         {
            SaveGraphDlg dlg = new SaveGraphDlg(getSite().getShell(), graphName, errorMessage, canBeOverwritten);
            result = dlg.open();
            if (result == Window.CANCEL)
               return;
            configuration.setName(dlg.getName());
            if (!canBeOverwritten)
               configuration.setId(0);
         }
         else
         {
            configuration.setName(graphName);
         }
      }    

      final GraphDefinition gs = asTemplate ? template : configuration;

      if (result == SaveGraphDlg.OVERRIDE)
      {
         new ConsoleJob(Messages.get().HistoricalGraphView_SaveSettings, this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               long id = session.saveGraph(gs, canBeOverwritten);
               if (!asTemplate)
                  configuration.setId(id);
            }

            @Override
            protected String getErrorMessage()
            {
               configuration.setId(oldGraphId);
               return Messages.get().HistoricalGraphView_SaveSettingsError;               
            }
         }.start();
      }
      else
      {
         new ConsoleJob(Messages.get().HistoricalGraphView_SaveSettings, this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               try
               {
                  long id = session.saveGraph(gs, canBeOverwritten);
                  if (!asTemplate)
                     configuration.setId(id);
               }
               catch(NXCException e)
               {
                  if (e.getErrorCode() == RCC.OBJECT_ALREADY_EXISTS)
                  {
                     runInUIThread(new Runnable() {
                        @Override
                        public void run()
                        {
                           configuration.setId(oldGraphId);
                           saveGraph(gs.getName(), Messages.get().HistoricalGraphView_NameAlreadyExist, true, asTemplate, true);
                        }
                     });
                  }
                  else
                  {
                     if (e.getErrorCode() == RCC.ACCESS_DENIED)
                     {
                        runInUIThread(new Runnable() {
                           @Override
                           public void run()
                           {
                              configuration.setId(oldGraphId);
                              saveGraph(gs.getName(), Messages.get().HistoricalGraphView_NameAlreadyExistNoOverwrite, false, asTemplate, true);
                           }
                        });
                     }
                     else
                     {
                        configuration.setId(oldGraphId);
                        throw e;
                     }
                  }
               }
            }

            @Override
            protected String getErrorMessage()
            {
               return Messages.get().HistoricalGraphView_SaveError;
            }
         }.start();
      }
   }

   /**
    * Create preset actions
    * 
    * @param handler
    * @return
    */
   public static Action[] createPresetActions(final PresetHandler handler)
   {
      Action[] actions = new Action[presetRanges.length];
      for(int i = 0; i < presetRanges.length; i++)
      {
         final Integer presetIndex = i;
         actions[i] = new Action(String.format(Messages.get().HistoricalGraphView_Last, presetNames[i])) {
            @Override
            public void run()
            {
               handler.onPresetSelected(presetUnits[presetIndex], presetRanges[presetIndex]);
            }
         };
      }
      return actions;
   }
   
   /**
    * Create action for chart
    * 
    * @param type
    * @param chartOwner
    * @return
    */
   public static Action createAction(ChartActionType type, final HistoricalChartOwner chartOwner)
   {
      Action action = null;
      switch(type)
      {
         case ADJUST_BOTH:
            action = new Action() {
               @Override
               public void run()
               {
                  chartOwner.getChart().adjustXAxis(false);
                  chartOwner.getChart().adjustYAxis(true);
               }
            };
            action.setText(Messages.get().HistoricalGraphView_Adjust);
            action.setImageDescriptor(Activator.getImageDescriptor("icons/adjust.png")); //$NON-NLS-1$
            break;
         case ADJUST_X:
            action = new Action() {
               @Override
               public void run()
               {
                  chartOwner.getChart().adjustXAxis(true);
               }
            };
            action.setText(Messages.get().HistoricalGraphView_AdjustX);
            action.setImageDescriptor(Activator.getImageDescriptor("icons/adjust_x.png")); //$NON-NLS-1$
            break;
         case ADJUST_Y:
            action = new Action() {
               @Override
               public void run()
               {
                  chartOwner.getChart().adjustYAxis(true);
               }
            };
            action.setText(Messages.get().HistoricalGraphView_AdjustY);
            action.setImageDescriptor(Activator.getImageDescriptor("icons/adjust_y.png")); //$NON-NLS-1$
            break;
      }
      return action;
   }

   /**
    * Action types
    */
   public enum ChartActionType
   {
      ADJUST_X, ADJUST_Y, ADJUST_BOTH
   }

   /**
    * Preset handler
    */
   public interface PresetHandler
   {
      /**
       * Called when new preset selected
       * 
       * @param unit time unit
       * @param range time range in units
       */
      public void onPresetSelected(TimeUnit unit, int range);
   }

   /**
    * Chart owner
    */
   public interface HistoricalChartOwner
   {
      /**
       * Get current chart object
       * 
       * @return current chart object
       */
      public Chart getChart();
   }

   /**
    * Show Graph configuration dialog
    * 
    * @param trap Object tool details object
    * @return true if OK was pressed
    */
   private boolean showGraphPropertyPages(final GraphDefinition settings)
   {
      PreferenceManager pm = new PreferenceManager();    
      pm.addToRoot(new PreferenceNode("graph", new Graph(settings, false)));
      pm.addToRoot(new PreferenceNode("general", new General(settings, false)));
      pm.addToRoot(new PreferenceNode("source", new DataSources(settings, false)));

      PreferenceDialog dlg = new PreferenceDialog(getViewSite().getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText("Properties for " + settings.getDisplayName());
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
   }
}
