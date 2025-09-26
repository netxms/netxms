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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.RefreshAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.datacollection.dialogs.SaveGraphDlg;
import org.netxms.nxmc.modules.datacollection.propertypages.DataSources;
import org.netxms.nxmc.modules.datacollection.propertypages.GeneralChart;
import org.netxms.nxmc.modules.datacollection.propertypages.Graph;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * History graph view
 */
public class HistoricalGraphView extends ViewWithContext implements ChartConfigurationChangeListener
{
   private final I18n i18n = LocalizationHelper.getI18n(HistoricalGraphView.class);
   private static final Logger logger = LoggerFactory.getLogger(HistoricalGraphView.class);

   private static final TimeUnit[] presetUnits = { TimeUnit.MINUTE, TimeUnit.MINUTE, TimeUnit.HOUR, TimeUnit.HOUR, TimeUnit.HOUR,
         TimeUnit.HOUR, TimeUnit.DAY, TimeUnit.DAY, TimeUnit.DAY, TimeUnit.DAY, TimeUnit.DAY, TimeUnit.DAY };
   private static final int[] presetRanges = { 10, 30, 1, 2, 4, 12, 1, 2, 5, 7, 31, 365 };
   private static final String[] presetNames = 
      { 
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last 10 minutes"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last 30 minutes"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last hour"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last 2 hours"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last 4 hours"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last 12 hours"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last day"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last 2 days"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last 5 days"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last week"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last month"),
         LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Last year") 
      };

   private long objectId;
   private long contextId;
   private String fullName;
   private NXCSession session = Registry.getSession();
   private Chart chart = null;
   private boolean updateInProgress = false;
   private ViewRefreshController refreshController;
   private Composite chartParent = null;
   private GraphDefinition configuration;
   private boolean multipleSourceNodes = false;
   private boolean showDeleteAction = false;

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
   private Action actionDelete;

   /**
    * Build view ID
    *
    * @param object context object
    * @param items list of DCIs to show
    * @return view ID
    */
   private static String buildId(AbstractObject object, List<ChartDciConfig> items)
   {
      StringBuilder sb = new StringBuilder("historical-graph");
      if (object != null)
      {
         sb.append('.');
         sb.append(object.getObjectId());
      }
      for(ChartDciConfig dci : items)
      {
         sb.append('.');
         sb.append(dci.dciId);
         sb.append('.');
         sb.append(dci.useRawValues);
      }
      return sb.toString();
   }

   /**
    * Create historical graph view with given context object and DCI list.
    *
    * @param contextObject context object
    * @param items initial set of DCIs to show
    * @param templateConfig template configuration (can be null)
    * @param items DCI list
    */
   public HistoricalGraphView(AbstractObject contextObject, List<ChartDciConfig> items, ChartConfiguration templateConfig, long contextId)
   {
      super(LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Line Chart"),
            ResourceManager.getImageDescriptor("icons/object-views/chart-line.png"), buildId(contextObject, items), false);
      objectId = contextObject.getObjectId();
      this.contextId = contextId;
      fullName = i18n.tr("Line Chart");

      configuration = new GraphDefinition();
      if (templateConfig != null)
      {
         configuration.setArea(templateConfig.isArea());
         configuration.setAutoScale(templateConfig.isAutoScale());
         configuration.setExtendedLegend(templateConfig.isExtendedLegend());
         configuration.setGridVisible(templateConfig.isGridVisible());
         configuration.setLogScale(templateConfig.isLogScale());
         configuration.setMaxYScaleValue(templateConfig.getMaxYScaleValue());
         configuration.setMinYScaleValue(templateConfig.getMinYScaleValue());
         configuration.setModifyYBase(templateConfig.isModifyYBase());
         configuration.setStacked(templateConfig.isStacked());
         configuration.setTitle(templateConfig.getTitle());
         configuration.setTranslucent(templateConfig.isTranslucent());
         configuration.setUseMultipliers(templateConfig.isUseMultipliers());
         configuration.setYAxisLabel(templateConfig.getYAxisLabel());
         configuration.setTimePeriod(templateConfig.getTimePeriod());
      }
      else
      {
         configuration.setTimePeriod(new TimePeriod(TimeFrameType.BACK_FROM_NOW, session.getClientConfigurationHintAsInt("DefaultLineChartPeriod", 60), TimeUnit.MINUTE, null, null));
      }

      configuration.setDciList(items.toArray(new ChartDciConfig[items.size()]));
      updateFullName(configuration.getDciList());
   }

   /**
    * Default constructor for use by cloneView()
    */
   public HistoricalGraphView()
   {
      super(LocalizationHelper.getI18n(HistoricalGraphView.class).tr("Line Chart"),
            ResourceManager.getImageDescriptor("icons/object-views/chart-line.png"), UUID.randomUUID().toString(), false); 
      fullName = i18n.tr("Line Chart");
   }

   /**
    * Constructor for use by graphs perspective
    */
   public HistoricalGraphView(GraphDefinition configuration)
   {
      super(configuration.getName(), ResourceManager.getImageDescriptor("icons/object-views/chart-line.png"), "historical-graph.predefined." + configuration.getId(), false);
      this.configuration = new GraphDefinition(configuration);
      this.fullName = configuration.getName();
      this.configuration.addChangeListener(this);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      HistoricalGraphView view = (HistoricalGraphView)super.cloneView();
      view.objectId = objectId;
      view.fullName = fullName;
      view.multipleSourceNodes = multipleSourceNodes;
      view.configuration = new GraphDefinition(configuration);
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#getFullName()
    */
   @Override
   public String getFullName()
   {
      return fullName;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return (context != null) && (context instanceof AbstractObject)
            && ((((AbstractObject)context).getObjectId() == objectId) || (((AbstractObject)context).getObjectId() == contextId));
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      if ((newContext == null) || !(newContext instanceof AbstractObject)
            || (((AbstractObject)newContext).getObjectId() != objectId))
         return;

      configuration.setTimeFrom(new Date(System.currentTimeMillis() - configuration.getTimeRangeMillis()));
      configuration.setTimeTo(new Date(System.currentTimeMillis()));

      configureGraphFromSettings();
   }

   /**
    * Memento to load context
    * 
    * @param memento
    */
   @Override
   public Object restoreContext(Memento memento)
   {      
      long objectId = memento.getAsLong("context", 0);
      return session.findObjectById(objectId);
   }   

   /**
    * @see org.netxms.nxmc.base.views.View#isCloseable()
    */
   @Override
   public boolean isCloseable()
   {
      return true;
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

      if (!configuration.getTitle().isBlank())
      {
         fullName = configuration.getTitle();
         setName(configuration.getTitle());
      }

      ChartConfiguration chartConfiguration = new ChartConfiguration(configuration);
      chart = new Chart(chartParent, SWT.NONE, ChartType.LINE, chartConfiguration, this);
      createPopupMenu();

      // Data
      int nodeId = 0;
      for(ChartDciConfig dci : configuration.getDciList())
      {
         nodeId |= dci.nodeId; // Check that all DCI's are from one node
         ChartDciConfig item = new ChartDciConfig(dci);
         if (configuration.isShowHostNames())
            item.name = session.getObjectName(dci.nodeId) + " - " + dci.getLabel();
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

   /**
    * Get DCI info (unit name and multiplier)
    */
   private void updateDciInfo()
   {
      Job job = new Job("Get DCI info", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (chart.isDisposed())
                     return;
                  
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
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      chartParent = parent;
      
      refreshController = new ViewRefreshController(this, -1, new Runnable() {
         @Override
         public void run()
         {
            if (((Widget)chart).isDisposed())
               return;

            updateChart();
         }
      });
      
      createActions();
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
      menuManager.addMenuListener((m) -> fillContextMenu(m));
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
      Job job = new Job(i18n.tr("Get DCI values for history graph"), this) {
         private ChartDciConfig currentItem;

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(getName(), dciList.length);
            final DataSeries[] data = new DataSeries[dciList.length];
            for(int i = 0; i < dciList.length; i++)
            {
               currentItem = dciList[i];
               if (currentItem.type == ChartDciConfig.ITEM)
               {
                  data[i] = session.getCollectedData(currentItem.nodeId, currentItem.dciId, configuration.getTimeFrom(),
                        configuration.getTimeTo(), 0,
                        currentItem.useRawValues ? HistoricalDataType.RAW : HistoricalDataType.PROCESSED);
               }
               else
               {
                  data[i] = session.getCollectedTableData(currentItem.nodeId, currentItem.dciId, currentItem.instance,
                        currentItem.column, configuration.getTimeFrom(), configuration.getTimeTo(), 0);
               }
               monitor.worked(1);
            }

            runInUIThread(() -> {
               if (!((Widget)chart).isDisposed())
               {
                  chart.setTimeRange(configuration.getTimeFrom(), configuration.getTimeTo());
                  setChartData(data);
                  clearMessages();
               }
               updateInProgress = false;
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot get value for DCI %s:\"%s\""), session.getObjectName(currentItem.nodeId), currentItem.name);
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            updateInProgress = false;
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            updateChart();
         }
      };

      actionProperties = new Action(i18n.tr("Properties")) {
         @Override
         public void run()
         {
            showGraphPropertyPages(configuration, getWindow().getShell());
            configureGraphFromSettings(); // Always refresh graph (last action was cancel, but before could have been apply actions)
         }
      };
      addKeyBinding("M3+ENTER", actionProperties);

      actionAutoRefresh = new Action(i18n.tr("Refresh &automatically")) {
         @Override
         public void run()
         {
            configuration.setAutoRefresh(!configuration.isAutoRefresh());
            setChecked(configuration.isAutoRefresh());
            refreshController.setInterval(configuration.isAutoRefresh() ? configuration.getRefreshRate() : -1);
         }
      };
      actionAutoRefresh.setChecked(configuration.isAutoRefresh());

      actionLogScale = new Action(i18n.tr("&Logarithmic scale")) {
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
               MessageDialogHelper.openError(getWindow().getShell(), i18n.tr("Error"),
                     String.format(i18n.tr("Cannot switch to logarithmic scale: %s"), e.getLocalizedMessage()));
               logger.error("Cannot change log scale mode", e);
            }
            setChecked(configuration.isLogScale());
         }
      };
      actionLogScale.setChecked(configuration.isLogScale());

      actionZoomIn = new Action(i18n.tr("Zoom &in")) {
         @Override
         public void run()
         {
            chart.zoomIn();
         }
      };
      actionZoomIn.setImageDescriptor(SharedIcons.ZOOM_IN);
      addKeyBinding("M1+=", actionZoomIn);

      actionZoomOut = new Action(i18n.tr("Zoom &out")) {
         @Override
         public void run()
         {
            chart.zoomOut();
         }
      };
      actionZoomOut.setImageDescriptor(SharedIcons.ZOOM_OUT);
      addKeyBinding("M1+-", actionZoomOut);

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
      addKeyBinding("M1+X", actionAdjustX);
      addKeyBinding("M1+Y", actionAdjustY);
      addKeyBinding("M1+0", actionAdjustBoth);

      actionShowLegend = new Action(i18n.tr("&Show legend")) {
         @Override
         public void run()
         {
            configuration.setLegendVisible(actionShowLegend.isChecked());
            chart.getConfiguration().setLegendVisible(configuration.isLegendVisible());
            chart.rebuild();
         }
      };
      actionShowLegend.setChecked(configuration.isLegendVisible());
      addKeyBinding("M1+M3+L", actionShowLegend);

      actionExtendedLegend = new Action(i18n.tr("&Extended legend")) {
         @Override
         public void run()
         {
            configuration.setExtendedLegend(actionExtendedLegend.isChecked());
            chart.getConfiguration().setExtendedLegend(configuration.isExtendedLegend());
            chart.rebuild();
         }
      };
      actionExtendedLegend.setChecked(configuration.isExtendedLegend());
      addKeyBinding("M1+M3+E", actionExtendedLegend);

      actionLegendLeft = new Action(i18n.tr("Place on the &left"), Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            configuration.setLegendPosition(ChartConfiguration.POSITION_LEFT);
            chart.getConfiguration().setLegendPosition(configuration.getLegendPosition());
            chart.rebuild();
         }
      };
      actionLegendLeft.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_LEFT);

      actionLegendRight = new Action(i18n.tr("Place on the &right"), Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            configuration.setLegendPosition(ChartConfiguration.POSITION_RIGHT);
            chart.getConfiguration().setLegendPosition(configuration.getLegendPosition());
            chart.rebuild();
         }
      };
      actionLegendRight.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_RIGHT);

      actionLegendTop = new Action(i18n.tr("Place on the &top"), Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            configuration.setLegendPosition(ChartConfiguration.POSITION_TOP);
            chart.getConfiguration().setLegendPosition(configuration.getLegendPosition());
            chart.rebuild();
         }
      };
      actionLegendTop.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_TOP);

      actionLegendBottom = new Action(i18n.tr("Place on the &bottom"), Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            configuration.setLegendPosition(ChartConfiguration.POSITION_BOTTOM);
            chart.getConfiguration().setLegendPosition(configuration.getLegendPosition());
            chart.rebuild();
         }
      };
      actionLegendBottom.setChecked(configuration.getLegendPosition() == ChartConfiguration.POSITION_BOTTOM);

      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            if (configuration.getId() == 0)
            {
               String initalName = configuration.getName().compareTo("noname") == 0 ? configuration.getTitle()
                     : configuration.getName();
               saveGraph(initalName, null, false, false, true);
            }
            else
            {
               saveGraph(configuration.getName(), null, false, false, false);
            }
         }
      };
      addKeyBinding("M1+S", actionSave);

      actionSaveAs = new Action(i18n.tr("Save &as..."), SharedIcons.SAVE_AS) {
         @Override
         public void run()
         {
            String initalName = configuration.getName().compareTo("noname") == 0 ? configuration.getTitle()
                  : configuration.getName();
            saveGraph(initalName, null, false, false, true);
         }
      };
      addKeyBinding("M1+M2+S", actionSaveAs);

      actionSaveAsTemplate = new Action(i18n.tr("Save as &template...")) {
         @Override
         public void run()
         {
            String initalName = configuration.getName().compareTo("noname") == 0 ? configuration.getTitle()
                  : configuration.getName();
            saveGraph(initalName, null, false, true, true);
         }
      };
      addKeyBinding("M1+T", actionSaveAsTemplate);

      actionStacked = new Action(i18n.tr("Sta&cked"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            configuration.setStacked(actionStacked.isChecked());
            configureGraphFromSettings();
         }
      };
      actionStacked.setChecked(configuration.isStacked());
      addKeyBinding("M1+M3+S", actionStacked);

      actionTranslucent = new Action(i18n.tr("&Translucent"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            configuration.setTranslucent(actionTranslucent.isChecked());
            configureGraphFromSettings();
         }
      };
      actionTranslucent.setChecked(configuration.isTranslucent());
      addKeyBinding("M1+M3+T", actionTranslucent);

      actionAreaChart = new Action(i18n.tr("Area chart"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            configuration.setArea(actionAreaChart.isChecked());
            configureGraphFromSettings();
         }
      };
      actionAreaChart.setChecked(configuration.isArea());
      addKeyBinding("M1+M3+A", actionAreaChart);

      presetActions = createPresetActions(new PresetHandler() {
         @Override
         public void onPresetSelected(TimeUnit unit, int range)
         {
            configuration.getTimePeriod().setTimeUnit(unit);
            configuration.getTimePeriod().setTimeRange(range);
            updateChart();
         }
      });

      actionCopyImage = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            chart.copyToClipboard();
         }
      };
      addKeyBinding("M1+C", actionCopyImage);

      actionSaveAsImage = new Action(i18n.tr("Save as &image..."), SharedIcons.SAVE_AS_IMAGE) {
         @Override
         public void run()
         {
            chart.saveAsImage(getWindow().getShell());
         }
      };
      addKeyBinding("M1+I", actionSaveAsImage);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deletePredefinedGraph();
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
    * Fill context menu
    * 
    * @param manager
    */
   private void fillContextMenu(IMenuManager manager)
   {
      MenuManager presets = new MenuManager(i18n.tr("&Presets"));
      for(int i = 0; i < presetActions.length; i++)
         presets.add(presetActions[i]);

      MenuManager legend = new MenuManager(i18n.tr("&Legend"));
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
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionSaveAsImage);
      manager.add(actionCopyImage);
      if (showDeleteAction)
      {
         manager.add(new Separator());
         manager.add(actionDelete);
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      MenuManager presets = new MenuManager(i18n.tr("&Presets"));
      for(int i = 0; i < presetActions.length; i++)
         presets.add(presetActions[i]);

      MenuManager legend = new MenuManager(i18n.tr("&Legend"));
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
      manager.add(actionSave);
      manager.add(actionSaveAs);
      manager.add(actionSaveAsTemplate);
      manager.add(actionSaveAsImage);
      manager.add(actionCopyImage);
      manager.add(new Separator());
      manager.add(actionRefresh);
      manager.add(new Separator());
      manager.add(actionProperties);
   }

   /**
    * Set chart data
    * 
    * @param data Retrieved DCI data
    */
   private void setChartData(final DataSeries[] data)
   {
      for(int i = 0; i < data.length; i++)
         chart.updateParameter(i, data[i], false);
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
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      updateChart();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
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
   private void saveGraph(String graphName, String errorMessage, final boolean canBeOverwritten, final boolean asTemplate,
         final boolean showNameDialog)
   {
      if (asTemplate && multipleSourceNodes)
      {
         String templateError = "More than one node is used for template creation.\nThis may cause undefined behaviour.";
         errorMessage = (errorMessage == null) ? templateError : errorMessage + "\n\n" + templateError;
      }

      int result = SaveGraphDlg.OK;
      GraphDefinition template = null;
      final long oldGraphId = configuration.getId();
      if (asTemplate)
      {
         SaveGraphDlg dlg = new SaveGraphDlg(getWindow().getShell(), graphName, errorMessage, canBeOverwritten);
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
            SaveGraphDlg dlg = new SaveGraphDlg(getWindow().getShell(), graphName, errorMessage, canBeOverwritten);
            result = dlg.open();
            if (result == Window.CANCEL)
               return;
            configuration.setName(dlg.getName());
            if (!canBeOverwritten)
               configuration.setId(0);
         }
         else
            configuration.setName(graphName);
      }

      final GraphDefinition gs = asTemplate ? template : configuration;

      if (result == SaveGraphDlg.OVERRIDE)
      {
         new Job(i18n.tr("Save graph settings"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               long id = session.saveGraph(gs, canBeOverwritten);
               if (!asTemplate)
                  configuration.setId(id);
            }

            @Override
            protected String getErrorMessage()
            {
               configuration.setId(oldGraphId);
               return i18n.tr("Cannot save graph settings as predefined graph");
            }
         }.start();
      }
      else
      {
         new Job(i18n.tr("Save graph settings"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
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
                     runInUIThread(() -> {
                        configuration.setId(oldGraphId);
                        saveGraph(gs.getName(), i18n.tr("Graph with given name already exists"), true, asTemplate, true);
                     });
                  }
                  else
                  {
                     if (e.getErrorCode() == RCC.ACCESS_DENIED)
                     {
                        runInUIThread(() -> {
                           configuration.setId(oldGraphId);
                           saveGraph(gs.getName(), i18n.tr("Graph with given name already exists and cannot be be overwritten"), false, asTemplate, true);
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
               return i18n.tr("Cannot save graph settings as predefined graph");
            }
         }.start();
      }
   }

   /**
    * Delete predefined graph(s)
    */
   private void deletePredefinedGraph()
   {
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Predefined Graphs"),
            i18n.tr("Predefined graphs will be deleted. Are you sure?")))
         return;

      new Job(String.format(i18n.tr("Delete predefined graph \"%s\""), configuration.getShortName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.deletePredefinedGraph(configuration.getId());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete predefined graph");
         }
      }.start();
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
         actions[i] = new Action(presetNames[i]) {
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
      I18n i18n = LocalizationHelper.getI18n(HistoricalGraphView.class);
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
            action.setText(i18n.tr("&Adjust"));
            action.setImageDescriptor(ResourceManager.getImageDescriptor("icons/adjust.png"));
            break;
         case ADJUST_X:
            action = new Action() {
               @Override
               public void run()
               {
                  chartOwner.getChart().adjustXAxis(true);
               }
            };
            action.setText(i18n.tr("Adjust &X axis"));
            action.setImageDescriptor(ResourceManager.getImageDescriptor("icons/adjust_x.png"));
            break;
         case ADJUST_Y:
            action = new Action() {
               @Override
               public void run()
               {
                  chartOwner.getChart().adjustYAxis(true);
               }
            };
            action.setText(i18n.tr("Adjust &Y axis"));
            action.setImageDescriptor(ResourceManager.getImageDescriptor("icons/adjust_y.png"));
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
    * @param showDeleteAction the showDeleteAction to set
    */
   public void setShowDeleteAction(boolean showDeleteAction)
   {
      this.showDeleteAction = showDeleteAction;
   }

   /**
    * Show Graph configuration dialog
    * 
    * @param trap Object tool details object
    * @return true if OK was pressed
    */
   public static boolean showGraphPropertyPages(final GraphDefinition settings, Shell parentShell)
   {
      I18n i18n = LocalizationHelper.getI18n(HistoricalGraphView.class);
      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("graph", new Graph(settings, false)));
      pm.addToRoot(new PreferenceNode("general", new GeneralChart(settings, false)));
      pm.addToRoot(new PreferenceNode("source", new DataSources(settings, false)));

      PreferenceDialog dlg = new PreferenceDialog(parentShell, pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText(String.format(i18n.tr("Properties for %s"), settings.getDisplayName()));
         }
      };
      dlg.setBlockOnOpen(true);
      return dlg.open() == Window.OK;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("objectId", objectId);
      memento.set("multipleSourceNodes", multipleSourceNodes);
      try
      {
         memento.set("configuration", XMLTools.serialize(configuration));
      }
      catch(Exception e)
      {
         logger.error("Failed to serialize configuration", e);
         memento.set("configuration", "");
      }
   }  

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      objectId = memento.getAsLong("editMode", 0);
      multipleSourceNodes = memento.getAsBoolean("multipleSourceNodes", true);
      try
      {
         configuration = XMLTools.createFromXml(GraphDefinition.class, memento.getAsString("configuration"));
      }
      catch(Exception e)
      {
         logger.error("Failed to load configuration", e);
         throw(new ViewNotRestoredException(i18n.tr("Failed to load configuration"), e));
      }
      updateFullName(configuration.getDciList());
   }

   /**
    * Update view full name
    *
    * @param chartDciConfigs DCI configurations
    */
   private void updateFullName(ChartDciConfig[] chartDciConfigs)
   {
      // Set view title to "host name: dci description" if we have only one DCI
      if (chartDciConfigs.length == 1)
      {
         ChartDciConfig item = chartDciConfigs[0];
         if (item.useRawValues)
         {
            setName(item.name.isEmpty() ? item.dciDescription : item.name + " " + i18n.tr("(raw)"));
         }
         else
         {
            setName(item.name.isEmpty() ? item.dciDescription : item.name);
         }
         AbstractObject object = session.findObjectById(item.nodeId);
         if (object != null)
         {
            fullName = object.getObjectName() + ": " + getName();
         }
         else
         {
            fullName = getName();
         }
      }
      else
      {
         long nodeId = chartDciConfigs[0].nodeId;
         for(ChartDciConfig item : chartDciConfigs)
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
               fullName = String.format(i18n.tr("%s: Line Chart"), object.getObjectName());
            }
         }
      }
   }
}
