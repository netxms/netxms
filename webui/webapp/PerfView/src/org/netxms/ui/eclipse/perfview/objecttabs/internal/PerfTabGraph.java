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
package org.netxms.ui.eclipse.perfview.objecttabs.internal;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.charts.api.ChartType;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.compatibility.GraphItem;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.PerfTabGraphSettings;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView.ChartActionType;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView.HistoricalChartOwner;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.ViewRefreshController;
import org.netxms.ui.eclipse.tools.VisibilityValidator;
import org.netxms.ui.eclipse.widgets.DashboardComposite;

/**
 * Performance tab graph
 */
public class PerfTabGraph extends DashboardComposite implements HistoricalChartOwner
{
	private long nodeId;
	private List<PerfTabDci> items = new ArrayList<PerfTabDci>(4);
   private Chart chart;
	private ViewRefreshController refreshController = null;
	private boolean updateInProgress = false;
	private NXCSession session;
	private IViewPart viewPart;
	private VisibilityValidator validator;
   private Action actionRefresh;
   private Action actionAdjustX;
   private Action actionAdjustY;
   private Action actionAdjustBoth;
   private Action[] presetActions;
   private PerfTabGraphSettings settings;

	/**
	 * @param parent
	 * @param style
	 */
	public PerfTabGraph(Composite parent, long nodeId, PerfTabDci dci, PerfTabGraphSettings settings, IViewPart viewPart, VisibilityValidator validator)
	{
		super(parent, SWT.BORDER);
		this.nodeId = nodeId;
		this.viewPart = viewPart;
		this.validator = validator;
		this.settings = settings;
		items.add(dci);
      session = ConsoleSharedData.getSession();

		setLayout(new FillLayout());

      ChartConfiguration chartConfiguration = new ChartConfiguration();
      chartConfiguration.setZoomEnabled(false);
      chartConfiguration.setTitleVisible(true);
      chartConfiguration.setTitle(settings.getRuntimeTitle());
      chartConfiguration.setLegendVisible(settings.isShowLegendAlways());
      chartConfiguration.setExtendedLegend(settings.isExtendedLegend());
      chartConfiguration.setLogScale(settings.isLogScaleEnabled());
      chartConfiguration.setUseMultipliers(settings.isUseMultipliers());
      chartConfiguration.setStacked(settings.isStacked());
      chartConfiguration.setTranslucent(settings.isTranslucent());
      chartConfiguration.setAutoScale(settings.isAutoScale());
      chartConfiguration.setMinYScaleValue(settings.getMinYScaleValue());
      chartConfiguration.setMaxYScaleValue(settings.getMaxYScaleValue());

      chart = new Chart(this, SWT.NONE, ChartType.LINE, chartConfiguration);

      final Date from = new Date(System.currentTimeMillis() - settings.getTimeRangeMillis());
      final Date to = new Date(System.currentTimeMillis());
      chart.setTimeRange(from, to);

      GraphItem item = new GraphItem(nodeId, dci.getId(), null, settings.getRuntimeName(), "", settings.getType(), settings.isAutomaticColor() ? -1 : settings.getColorAsInt());
      item.setInverted(settings.isInvertedValues());
      item.setShowThresholds(settings.isShowThresholds());
      item.setMeasurementUnit(dci.getMeasurementUnit());
      chart.addParameter(item);

		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (refreshController != null)
               refreshController.dispose();
         }
      });

      chart.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            openHistoryGraph();
         }
      });

      createActions();
      createChartContextMenu();
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
            settings.setTimeUnits(unit.getValue());
            settings.setTimeRange(range);
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
	 * Add another item to graph
	 * 
	 * @param dci
	 * @param settings
	 */
	public void addItem(PerfTabDci dci, PerfTabGraphSettings settings)
	{
      chart.getConfiguration().setLegendVisible(true);
		if (settings.isExtendedLegend())
         chart.getConfiguration().setExtendedLegend(true);
		synchronized(items)
		{
			items.add(dci);
         GraphItem item = new GraphItem(nodeId, dci.getId(), null, settings.getRuntimeName(), "", settings.getType(), settings.isAutomaticColor() ? -1 : settings.getColorAsInt());
         item.setInverted(settings.isInvertedValues());
         item.setShowThresholds(settings.isShowThresholds());
         item.setMeasurementUnit(dci.getMeasurementUnit());
         chart.addParameter(item);
		}
	}

	/**
	 * Start chart update
	 */
	public void start()
	{
      chart.rebuild();
		refreshController = new ViewRefreshController(viewPart, 30, new Runnable() {
			@Override
			public void run()
			{
				if (PerfTabGraph.this.isDisposed())
					return;

				refreshData();
			}
		}, validator);
		if (validator.isVisible())
		   refreshData();
	}

	/**
	 * Refresh graph's data
	 */
	public void refreshData()
	{
		if (updateInProgress)
			return;

		updateInProgress = true;
		chart.clearErrors();

		ConsoleJob job = new ConsoleJob(Messages.get().PerfTabGraph_JobTitle, null, Activator.PLUGIN_ID) {
			private PerfTabDci currentDci;

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Date from = new Date(System.currentTimeMillis() - settings.getTimeRangeMillis());
				final Date to = new Date(System.currentTimeMillis());
				synchronized(items)
				{
					final DciData[] data = new DciData[items.size()];
	            final Threshold[][] thresholds = new Threshold[items.size()][];
					for(int i = 0; i < data.length; i++)
					{
						currentDci = items.get(i);
						data[i] = session.getCollectedData(nodeId, currentDci.getId(), from, to, 0, HistoricalDataType.PROCESSED);
                  thresholds[i] = session.getThresholds(nodeId, currentDci.getId());
					}
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
                     if (!chart.isDisposed())
							{
								chart.setTimeRange(from, to);
								for(int i = 0; i < data.length; i++)
                           chart.updateParameter(i, data[i], false);
                        chart.setThresholds(thresholds);
                        chart.refresh();
							}
							updateInProgress = false;
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format(Messages.get().PerfTabGraph_JobError, currentDci.getId(), currentDci.getDescription());
			}

			@Override
			protected void jobFailureHandler()
			{
				updateInProgress = false;
				super.jobFailureHandler();
			}

			@Override
			protected IStatus createFailureStatus(Exception e)
			{
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
					   if (!((Widget)chart).isDisposed())
					      chart.addError(getErrorMessage());
					}
				});
				return Status.OK_STATUS;
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * Open history graph of dci
	 */
	private void openHistoryGraph()
	{
	   StringBuilder sb = new StringBuilder();
	   
	   for(int i = 0; i < items.size() && i < ChartConfiguration.MAX_GRAPH_ITEM_COUNT; i++)
	   {
	      sb.append("&");
         sb.append(Integer.toString(ChartDciConfig.ITEM));
         sb.append("@");
         sb.append(nodeId);
         sb.append("@");
         sb.append(items.get(i).getId());
         sb.append("@");
         try
         {
            sb.append(URLEncoder.encode(items.get(i).getDescription(), "UTF-8"));
         }
         catch(UnsupportedEncodingException e)
         {
            sb.append("<description unavailable>");
         }
         sb.append("@");
         sb.append("");
         sb.append("@");
         sb.append(false);
         sb.append("@");
         sb.append(chart.getItem(i).isInverted());
         sb.append("@");
         sb.append(chart.getItem(i).isArea(false) ? ChartDciConfig.AREA : ChartDciConfig.LINE);
         sb.append("@");
         sb.append(chart.getItem(i).getColor());
         sb.append("@");
         sb.append(settings.isStacked());
	   }

	   IWorkbenchPage page = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage();
	   try
	   {
	      page.showView(HistoricalGraphView.ID, sb.toString(), IWorkbenchPage.VIEW_ACTIVATE);
	   }
	   catch(PartInitException e)
	   {
	      MessageDialogHelper.openError(getShell(), "Error",
	            "Could not open history graph view " + e.getLocalizedMessage());
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
