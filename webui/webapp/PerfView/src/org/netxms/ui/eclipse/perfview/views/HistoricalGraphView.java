/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.datacollection.GraphSettingsChangeListener;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartDciConfig;
import org.netxms.ui.eclipse.charts.api.ChartFactory;
import org.netxms.ui.eclipse.charts.api.HistoricalDataChart;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.ChartConfig;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.dialogs.SaveGraphDlg;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * History graph view
 */
@SuppressWarnings("restriction")
public class HistoricalGraphView extends ViewPart implements GraphSettingsChangeListener
{
   public static final String ID = "org.netxms.ui.eclipse.perfview.views.HistoryGraph"; //$NON-NLS-1$
   public static final String PREDEFINED_GRAPH_SUBID = "org.netxms.ui.eclipse.charts.predefinedGraph"; //$NON-NLS-1$

   private static final int[] presetUnits = { GraphSettings.TIME_UNIT_MINUTE, GraphSettings.TIME_UNIT_MINUTE,
         GraphSettings.TIME_UNIT_HOUR, GraphSettings.TIME_UNIT_HOUR, GraphSettings.TIME_UNIT_HOUR, GraphSettings.TIME_UNIT_HOUR,
         GraphSettings.TIME_UNIT_DAY, GraphSettings.TIME_UNIT_DAY, GraphSettings.TIME_UNIT_DAY, GraphSettings.TIME_UNIT_DAY,
         GraphSettings.TIME_UNIT_DAY, GraphSettings.TIME_UNIT_DAY };
   private static final int[] presetRanges = { 10, 30, 1, 2, 4, 12, 1, 2, 5, 7, 31, 365 };
   private final String[] presetNames = 
      { Messages.get().HistoricalGraphView_Preset10min, Messages.get().HistoricalGraphView_Preset30min, Messages.get().HistoricalGraphView_Preset1hour, Messages.get().HistoricalGraphView_Preset2hours, Messages.get().HistoricalGraphView_Preset4hours, Messages.get().HistoricalGraphView_Preset12hours, Messages.get().HistoricalGraphView_Preset1day,
        Messages.get().HistoricalGraphView_Preset2days, Messages.get().HistoricalGraphView_Preset5days, Messages.get().HistoricalGraphView_PresetWeek, Messages.get().HistoricalGraphView_PresetMonth, Messages.get().HistoricalGraphView_PresetYear };

   private NXCSession session;
   private HistoricalDataChart chart = null;
   private boolean updateInProgress = false;
   private Runnable refreshTimer;
   private Composite chartParent = null;

   private GraphSettings settings = new GraphSettings();
   private ChartConfig config = new ChartConfig();

   private Action actionRefresh;
   private Action actionAutoRefresh;
   private Action actionZoomIn;
   private Action actionZoomOut;
   private Action actionAdjustX;
   private Action actionAdjustY;
   private Action actionAdjustBoth;
   private Action actionLogScale;
   private Action actionStacked;
   private Action actionTranslucent;
   private Action actionShowLegend;
   private Action actionExtendedLegend;
   private Action actionLegendLeft;
   private Action actionLegendRight;
   private Action actionLegendTop;
   private Action actionLegendBottom;
   private Action actionProperties;
   private Action actionSave;
   private Action[] presetActions;

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);

      final Display display = site.getShell().getDisplay();
      refreshTimer = new Runnable() {
         @Override
         public void run()
         {
            if (((Widget)chart).isDisposed())
               return;

            updateChart();
            display.timerExec(-1, this);
            display.timerExec(config.getRefreshRate() * 1000, this);
         }
      };

      session = (NXCSession)ConsoleSharedData.getSession();

      config.setTimeFrom(new Date(System.currentTimeMillis() - config.getTimeRangeMillis()));
      config.setTimeTo(new Date(System.currentTimeMillis()));

      // Extract DCI ids from view id
      // (first field will be unique view id, so we skip it)
      String id = site.getSecondaryId();
      String[] fields = id.split("&"); //$NON-NLS-1$
      if (!fields[0].equals(PREDEFINED_GRAPH_SUBID))
      {
         List<ChartDciConfig> items = new ArrayList<ChartDciConfig>();
         for(int i = 1; i < fields.length; i++)
         {
            String[] subfields = fields[i].split("\\@"); //$NON-NLS-1$
            if (subfields.length == 6) // item
            {
               try
               {
                  ChartDciConfig dci = new ChartDciConfig();
                  dci.nodeId = Long.parseLong(subfields[0], 10);
                  dci.dciId = Long.parseLong(subfields[1], 10);
                  dci.name = URLDecoder.decode(subfields[5], "UTF-8"); //$NON-NLS-1$
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
            else if (subfields.length == 8) // table
            {
               try
               {
                  ChartDciConfig dci = new ChartDciConfig();
                  dci.type = ChartDciConfig.TABLE;
                  dci.nodeId = Long.parseLong(subfields[0], 10);
                  dci.dciId = Long.parseLong(subfields[1], 10);
                  dci.name = URLDecoder.decode(subfields[5], "UTF-8"); //$NON-NLS-1$
                  dci.instance = URLDecoder.decode(subfields[6], "UTF-8"); //$NON-NLS-1$
                  dci.column = URLDecoder.decode(subfields[7], "UTF-8"); //$NON-NLS-1$
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
         config.setTitle(getPartName());
         config.setDciList(items.toArray(new ChartDciConfig[items.size()]));
      }
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
    */
   @Override
   public void init(IViewSite site, IMemento memento) throws PartInitException
   {
      init(site);

      if (memento != null)
      {
         try
         {
            config = ChartConfig.createFromXml(memento.getTextData());
         }
         catch(Exception e)
         {
            e.printStackTrace();
         }
      }
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.ViewPart#saveState(org.eclipse.ui.IMemento)
    */
   @Override
   public void saveState(IMemento memento)
   {
      try
      {
         memento.putTextData(config.createXml());
      }
      catch(Exception e)
      {
      }
   }

   /**
    * Initialize this view with predefined graph settings
    * 
    * @param gs graph settings
    */
   public void initPredefinedGraph(GraphSettings gs)
   {
      settings = gs;
      try
      {
         config = ChartConfig.createFromXml(settings.getConfig());
      }
      catch(Exception e)
      {
         e.printStackTrace();
      }
      settings.addChangeListener(this);
      configureGraphFromSettings();
   }

   /**
    * Configure graph from graph settings
    */
   private void configureGraphFromSettings()
   {
      if (chart != null)
         ((Widget)chart).dispose();
      chart = ChartFactory.createLineChart(chartParent, SWT.NONE);
      createPopupMenu();

      // General settings
      setPartName(config.getTitle());
      chart.setChartTitle(config.getTitle());

      // Chart visual settings
      chart.setLogScaleEnabled(config.isLogScale());
      chart.setGridVisible(config.isShowGrid());
      chart.setLegendVisible(config.isShowLegend());
      chart.setLegendPosition(config.getLegendPosition());
      chart.setStacked(config.isStacked());
      chart.setTranslucent(config.isTranslucent());

      // Data
      final List<GraphItemStyle> styles = new ArrayList<GraphItemStyle>(config.getDciList().length);
      int index = 0;
      for(ChartDciConfig dci : config.getDciList())
      {
         final String name = config.isShowHostNames() ? (session.getObjectName(dci.nodeId) + " - " + dci.getName()) : dci.getName(); //$NON-NLS-1$
         chart.addParameter(new GraphItem(dci.nodeId, dci.dciId, 0, 0, Long.toString(dci.dciId), name));
         int color = dci.getColorAsInt();
         if (color == -1)
            color = ChartColor.getDefaultColor(index).getRGB();
         styles.add(new GraphItemStyle(dci.area ? GraphItemStyle.AREA : GraphItemStyle.LINE, color, 2, 0));
         index++;
      }
      chart.setItemStyles(styles);

      if (config.getTimeFrameType() == GraphSettings.TIME_FRAME_BACK_FROM_NOW)
      {
         config.setTimeFrom(new Date(System.currentTimeMillis() - config.getTimeRangeMillis()));
         config.setTimeTo(new Date(System.currentTimeMillis()));
      }

      getDataFromServer();

      // Automatic refresh
      actionAutoRefresh.setChecked(config.isAutoRefresh());
      getSite().getShell().getDisplay().timerExec(-1, refreshTimer);
		if (config.isAutoRefresh())
		{
		   getSite().getShell().getDisplay().timerExec(config.getRefreshRate() * 1000, refreshTimer);
	
		}
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      chartParent = parent;

      createActions();
      contributeToActionBars();

      configureGraphFromSettings();
      settings.addChangeListener(this);
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu((Control)chart);
      ((Control)chart).setMenu(menu);
      for(Control ch : ((Composite)chart).getChildren())
      {
         ch.setMenu(menu);
      }
   }

   /**
    * Get DCI data from server
    */
   private void getDataFromServer()
   {
      final ChartDciConfig[] dciList = config.getDciList();

      // Request data from server
      ConsoleJob job = new ConsoleJob(Messages.get().HistoricalGraphView_JobName, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
         private ChartDciConfig currentItem;

         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            monitor.beginTask(getName(), dciList.length);
            final DciData[] data = new DciData[dciList.length];
            final Threshold[][] thresholds = new Threshold[dciList.length][];
            for(int i = 0; i < dciList.length; i++)
            {
               currentItem = dciList[i];
               if (currentItem.type == ChartDciConfig.ITEM)
               {
                  data[i] = session.getCollectedData(currentItem.nodeId, currentItem.dciId, config.getTimeFrom(),
                        config.getTimeTo(), 0);
                  thresholds[i] = session.getThresholds(currentItem.nodeId, currentItem.dciId);
               }
               else
               {
                  data[i] = session.getCollectedTableData(currentItem.nodeId, currentItem.dciId, currentItem.instance,
                        currentItem.column, config.getTimeFrom(), config.getTimeTo(), 0);
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
                     chart.setTimeRange(config.getTimeFrom(), config.getTimeTo());
                     setChartData(data);
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

   /*
    * (non-Javadoc)
    * 
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
            PropertyDialog dlg = PropertyDialog.createDialogOn(getSite().getShell(), null, config);
            if (dlg != null)
            {
               dlg.open();
               configureGraphFromSettings();
            }
         }
      };

      actionAutoRefresh = new Action(Messages.get().HistoricalGraphView_Autorefresh) {
         @Override
         public void run()
         {
            config.setAutoRefresh(!config.isAutoRefresh());
            setChecked(config.isAutoRefresh());
            HistoricalGraphView.this.getSite().getShell().getDisplay().timerExec(-1, refreshTimer);
				if (config.isAutoRefresh())
				{
				   HistoricalGraphView.this.getSite().getShell().getDisplay().timerExec(config.getRefreshRate() * 1000, refreshTimer);
				}
         }
      };
      actionAutoRefresh.setChecked(config.isAutoRefresh());

      actionLogScale = new Action(Messages.get().HistoricalGraphView_LogScale) {
         @Override
         public void run()
         {
            try
            {
               chart.setLogScaleEnabled(!config.isLogScale());
               config.setLogScale(!config.isLogScale());
            }
            catch(IllegalStateException e)
            {
               MessageDialogHelper.openError(getSite().getShell(), Messages.get().HistoricalGraphView_Error,
                     String.format(Messages.get().HistoricalGraphView_LogScaleSwitchError, e.getLocalizedMessage()));
            }
            setChecked(config.isLogScale());
         }
      };
      actionLogScale.setChecked(config.isLogScale());

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

      actionAdjustX = new Action() {
         @Override
         public void run()
         {
            chart.adjustXAxis(true);
         }
      };
      actionAdjustX.setText(Messages.get().HistoricalGraphView_AdjustX);
      actionAdjustX.setImageDescriptor(Activator.getImageDescriptor("icons/adjust_x.png")); //$NON-NLS-1$

      actionAdjustY = new Action() {
         @Override
         public void run()
         {
            chart.adjustYAxis(true);
         }
      };
      actionAdjustY.setText(Messages.get().HistoricalGraphView_AdjustY);
      actionAdjustY.setImageDescriptor(Activator.getImageDescriptor("icons/adjust_y.png")); //$NON-NLS-1$

      actionAdjustBoth = new Action() {
         @Override
         public void run()
         {
            chart.adjustXAxis(false);
            chart.adjustYAxis(true);
         }
      };
      actionAdjustBoth.setText(Messages.get().HistoricalGraphView_Adjust);
      actionAdjustBoth.setImageDescriptor(Activator.getImageDescriptor("icons/adjust.png")); //$NON-NLS-1$

      actionShowLegend = new Action(Messages.get().HistoricalGraphView_ShowLegend) {
         @Override
         public void run()
         {
            config.setShowLegend(actionShowLegend.isChecked());
            chart.setLegendVisible(config.isShowLegend());
         }
      };
      actionShowLegend.setChecked(config.isShowLegend());

      actionExtendedLegend = new Action(Messages.get().HistoricalGraphView_ExtendedLegend) {
         @Override
         public void run()
         {
            config.setExtendedLegend(actionExtendedLegend.isChecked());
            chart.setExtendedLegend(config.isExtendedLegend());
         }
      };
      actionExtendedLegend.setChecked(config.isExtendedLegend());
      
      actionLegendLeft = new Action(Messages.get().HistoricalGraphView_PlaceOnLeft, Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            config.setLegendPosition(GraphSettings.POSITION_LEFT);
            chart.setLegendPosition(config.getLegendPosition());
         }
      };
      actionLegendLeft.setChecked(config.getLegendPosition() == GraphSettings.POSITION_LEFT);

      actionLegendRight = new Action(Messages.get().HistoricalGraphView_PlaceOnRight, Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            config.setLegendPosition(GraphSettings.POSITION_RIGHT);
            chart.setLegendPosition(config.getLegendPosition());
         }
      };
      actionLegendRight.setChecked(config.getLegendPosition() == GraphSettings.POSITION_RIGHT);

      actionLegendTop = new Action(Messages.get().HistoricalGraphView_PlaceOnTop, Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            config.setLegendPosition(GraphSettings.POSITION_TOP);
            chart.setLegendPosition(config.getLegendPosition());
         }
      };
      actionLegendTop.setChecked(config.getLegendPosition() == GraphSettings.POSITION_TOP);

      actionLegendBottom = new Action(Messages.get().HistoricalGraphView_PlaceOnBottom, Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            config.setLegendPosition(GraphSettings.POSITION_BOTTOM);
            chart.setLegendPosition(config.getLegendPosition());
         }
      };
      actionLegendBottom.setChecked(config.getLegendPosition() == GraphSettings.POSITION_BOTTOM);

      actionSave = new Action(Messages.get().HistoricalGraphView_Save, SharedIcons.SAVE) {
         @Override
         public void run()
         {
            String initalName = config.getTitle();
            saveGraph(initalName, null, false);
         }
      };

      actionStacked = new Action(Messages.get().HistoricalGraphView_Stacked, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            config.setStacked(actionStacked.isChecked());
            configureGraphFromSettings();
         }
      };
      actionStacked.setChecked(config.isStacked());

      actionTranslucent = new Action(Messages.get().HistoricalGraphView_Translucent, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            config.setTranslucent(actionTranslucent.isChecked());
            configureGraphFromSettings();
         }
      };
      actionTranslucent.setChecked(config.isTranslucent());

      presetActions = new Action[presetRanges.length];
      for(int i = 0; i < presetRanges.length; i++)
      {
         final Integer presetIndex = i;
         presetActions[i] = new Action(String.format(Messages.get().HistoricalGraphView_Last, presetNames[i])) {
            @Override
            public void run()
            {
               config.setTimeUnits(presetUnits[presetIndex]);
               config.setTimeRange(presetRanges[presetIndex]);
               updateChart();
            }
         };
      }
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
      manager.add(actionStacked);
      manager.add(actionLogScale);
      manager.add(actionTranslucent);
      manager.add(actionAutoRefresh);
      manager.add(legend);
      manager.add(new Separator());
      manager.add(actionRefresh);
      manager.add(new Separator());
      manager.add(actionSave);
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
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Set chart data
    * 
    * @param data Retrieved DCI data
    */
   private void setChartData(final DciData[] data)
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
      config.setTimeFrom(new Date(System.currentTimeMillis() - config.getTimeRangeMillis()));
      config.setTimeTo(new Date(System.currentTimeMillis()));
      getDataFromServer();
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      getSite().getShell().getDisplay().timerExec(-1, refreshTimer);
      super.dispose();
   }

   /*
    * (non-Javadoc)
    * 
    * @see
    * org.netxms.client.datacollection.GraphSettingsChangeListener#onGraphSettingsChange(org.netxms.client.datacollection.GraphSettings
    * )
    */
   @Override
   public void onGraphSettingsChange(GraphSettings settings)
   {
      if (this.settings == settings)
         configureGraphFromSettings();
   }

   /**
    * Save this graph as predefined
    */
   private void saveGraph(String graphName, String errorMessage, final boolean canBeOverwritten)
   {
      SaveGraphDlg dlg = new SaveGraphDlg(getSite().getShell(), graphName, errorMessage, canBeOverwritten);
      int result = dlg.open();
      if (result == Window.CANCEL)
         return;

      final GraphSettings gs = new GraphSettings(0, session.getUserId(), new ArrayList<AccessListElement>(0));
      gs.setName(dlg.getName());
      try
      {
         gs.setConfig(config.createXml());
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getSite().getShell(), "Internal Error", "Enexpected exception: " + e.getLocalizedMessage()); //$NON-NLS-1$ //$NON-NLS-2$
      }

      if (result == SaveGraphDlg.OVERRIDE)
      {
         new ConsoleJob(Messages.get().HistoricalGraphView_SaveSettings, this, Activator.PLUGIN_ID, null) {
            @Override
            protected void runInternal(IProgressMonitor monitor) throws Exception
            {
               session.saveGraph(gs, canBeOverwritten);
            }

            @Override
            protected String getErrorMessage()
            {
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
                  session.saveGraph(gs, canBeOverwritten);
               }
               catch(NXCException e)
               {
                  if (e.getErrorCode() == RCC.OBJECT_ALREADY_EXISTS)
                  {
                     runInUIThread(new Runnable() {

                        @Override
                        public void run()
                        {
                           saveGraph(gs.getName(), Messages.get().HistoricalGraphView_NameAlreadyExist, true);
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
                              saveGraph(gs.getName(), Messages.get().HistoricalGraphView_NameAlreadyExistNoOverwrite, false);
                           }

                        });
                     }
                     else
                     {
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
      updateChart();
   }
}
