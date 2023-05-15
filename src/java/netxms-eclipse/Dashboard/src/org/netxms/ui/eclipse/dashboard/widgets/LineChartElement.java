/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.ui.IViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.MeasurementUnit;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.charts.api.ChartType;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LineChartConfig;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView.ChartActionType;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView.HistoricalChartOwner;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ViewRefreshController;

/**
 * Line chart element
 */
public class LineChartElement extends ElementWidget implements HistoricalChartOwner
{
   private Chart chart;
	private LineChartConfig config;
	private ViewRefreshController refreshController;
	private boolean updateInProgress = false;
	private NXCSession session;
   private List<ChartDciConfig> runtimeDciList = new ArrayList<>();
	private List<DataCacheElement> dataCache = new ArrayList<DataCacheElement>(16);
   private Action actionRefresh;
   private Action actionAdjustX;
   private Action actionAdjustY;
   private Action actionAdjustBoth;
   private Action[] presetActions;

	/**
	 * @param parent
	 * @param data
	 */
	public LineChartElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);
      session = ConsoleSharedData.getSession();

		try
		{
			config = LineChartConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new LineChartConfig();
		}

      processCommonSettings(config);

      ChartConfiguration chartConfig = new ChartConfiguration();
      chartConfig.setZoomEnabled(config.isInteractive());
      chartConfig.setTitleVisible(false);
      chartConfig.setLegendVisible(config.isShowLegend());
      chartConfig.setLegendPosition(config.getLegendPosition());
      chartConfig.setExtendedLegend(config.isExtendedLegend());
      chartConfig.setGridVisible(config.isShowGrid());
      chartConfig.setLogScale(config.isLogScaleEnabled());
      chartConfig.setStacked(config.isStacked());
      chartConfig.setLineWidth(config.getLineWidth());
      chartConfig.setArea(config.isArea());
      chartConfig.setTranslucent(config.isTranslucent());
      chartConfig.setUseMultipliers(config.isUseMultipliers());
      chartConfig.setAutoScale(config.isAutoScale());
      chartConfig.setMinYScaleValue(config.getMinYScaleValue());
      chartConfig.setMaxYScaleValue(config.getMaxYScaleValue());
      chartConfig.setModifyYBase(config.modifyYBase());

      chart = new Chart(getContentArea(), SWT.NONE, ChartType.LINE, chartConfig);

		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (refreshController != null)
               refreshController.dispose();
         }
      });

		if (config.isInteractive())
		{
		   createActions();
		   createChartContextMenu();
		}

      configureMetrics();
	}   

	/**
    * Configure metrics on chart
    */
   private void configureMetrics()
   {
      ConsoleJob job = new ConsoleJob("Get DCI info", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            DciValue[] nodeDciList = null;
            for(ChartDciConfig dci : config.getDciList())
            {
               if ((dci.nodeId == 0) || (dci.nodeId == AbstractObject.CONTEXT))
               {
                  AbstractObject contextObject = getContext();
                  if (contextObject == null)
                     continue;

                  if (nodeDciList == null)
                     nodeDciList = session.getLastValues(contextObject.getObjectId());

                  Pattern namePattern = Pattern.compile(dci.dciName);
                  Pattern descriptionPattern = Pattern.compile(dci.dciDescription);
                  for(DciValue dciInfo : nodeDciList)
                  {
                     if ((!dci.dciName.isEmpty() && namePattern.matcher(dciInfo.getName()).find()) ||
                         (!dci.dciDescription.isEmpty() && descriptionPattern.matcher(dciInfo.getDescription()).find()))
                     {
                        ChartDciConfig instance = new ChartDciConfig(dci, dciInfo);
                        runtimeDciList.add(instance);
                        if (!dci.multiMatch)
                           break;
                     }
                  }
               }
               else
               {
                  runtimeDciList.add(dci);
               }
            }

            final Map<Long, MeasurementUnit> measurementUnits = session.getDciMeasurementUnits(runtimeDciList);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (chart.isDisposed())
                     return;

                  for(ChartDciConfig dci : runtimeDciList)
                  {
                     GraphItem item = new GraphItem(dci);
                     item.setMeasurementUnit(measurementUnits.get(dci.getDciId()));
                     chart.addParameter(item);
                  }

                  chart.rebuild();
                  layout(true, true);          
                  refreshData();

                  refreshController = new ViewRefreshController(viewPart, config.getRefreshRate(), new Runnable() {
                     @Override
                     public void run()
                     {
                        if (LineChartElement.this.isDisposed())
                           return;

                        refreshData();
                     }
                  });
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
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {
            refreshData();
         }
      };

      actionAdjustX = HistoricalGraphView.createAction(ChartActionType.ADJUST_X, this);
      actionAdjustY = HistoricalGraphView.createAction(ChartActionType.ADJUST_Y, this);
      actionAdjustBoth = HistoricalGraphView.createAction(ChartActionType.ADJUST_BOTH, this);

      presetActions = HistoricalGraphView.createPresetActions(new HistoricalGraphView.PresetHandler() {
         @Override
         public void onPresetSelected(TimeUnit unit, int range)
         {
            config.setTimeUnits(unit.getValue());
            config.setTimeRange(range);
            refreshData();
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
	   manager.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(manager);
         }
      });
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
      manager.add(new Separator());
      manager.add(actionRefresh);
	}

	/**
	 * Refresh graph's data
	 */
	private void refreshData()
	{
		if (updateInProgress)
			return;
		
		updateInProgress = true;
		
		ConsoleJob job = new ConsoleJob(Messages.get().LineChartElement_JobTitle, viewPart, Activator.PLUGIN_ID) {
			private ChartDciConfig currentDci;

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Date from = new Date(System.currentTimeMillis() - config.getTimeRangeMillis());
				final Date to = new Date(System.currentTimeMillis());
            final DciData[] data = new DciData[runtimeDciList.size()];
            final Threshold[][] thresholds = new Threshold[runtimeDciList.size()][];
            for(int i = 0; i < runtimeDciList.size(); i++)
            {
               currentDci = runtimeDciList.get(i);
               if (currentDci.type == ChartDciConfig.ITEM)
               {
                  data[i] = session.getCollectedData(currentDci.nodeId, currentDci.dciId, from, to, 0, HistoricalDataType.PROCESSED);
                  thresholds[i] = session.getThresholds(currentDci.nodeId, currentDci.dciId);
               }
               else
               {
                  data[i] = session.getCollectedTableData(currentDci.nodeId, currentDci.dciId, currentDci.instance, currentDci.column, from, to, 0);
                  thresholds[i] = null;
               }
            }
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!chart.isDisposed())
                  {
                     dataCache.clear();
                     chart.setTimeRange(from, to);
                     for(int i = 0; i < data.length; i++)
                     {
                        chart.updateParameter(i, data[i], false);
                        dataCache.add(new DataCacheElement(runtimeDciList.get(i), data[i]));
                     }
                     chart.setThresholds(thresholds);
                     chart.refresh();
                     chart.clearErrors();
                  }
                  updateInProgress = false;
               }
            });
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format(Messages.get().LineChartElement_JobError, session.getObjectName(currentDci.nodeId), currentDci.name);
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
	 * Get data cache for this chart. Must be called on UI thread.
	 * 
	 * @return
	 */
	public List<DataCacheElement> getDataCache()
	{
	   return new ArrayList<DataCacheElement>(dataCache);
	}
	
	/**
	 * Data cache element
	 */
	public class DataCacheElement
	{
	   public DciData data;
	   public String name;
	   
	   public DataCacheElement(ChartDciConfig config, DciData data)
	   {
	      this.name = config.getLabel();
	      this.data = data;
	   }
	}

   /**
    * @see org.netxms.ui.eclipse.perfview.views.HistoricalGraphView.HistoricalChartOwner#getChart()
    */
   @Override
   public Chart getChart()
   {
      return chart;
   }
}
