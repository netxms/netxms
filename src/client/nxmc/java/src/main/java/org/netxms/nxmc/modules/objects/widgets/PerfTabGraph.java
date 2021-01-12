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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Widget;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.RefreshAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.DashboardComposite;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartFactory;
import org.netxms.nxmc.modules.charts.api.HistoricalChartOwner;
import org.netxms.nxmc.modules.charts.api.HistoricalDataChart;
import org.netxms.nxmc.modules.charts.api.PerfTabGraphSettings;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.xnap.commons.i18n.I18n;

/**
 * Performance tab graph
 *
 */
public class PerfTabGraph extends DashboardComposite implements HistoricalChartOwner
{
   private static I18n i18n = LocalizationHelper.getI18n(PerfTabGraph.class);

	private long nodeId;
	private List<PerfTabDci> items = new ArrayList<PerfTabDci>(4);
	private HistoricalDataChart chart;
   private View view;
	private ViewRefreshController refreshController = null;
	private boolean updateInProgress = false;
	private NXCSession session;
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
	public PerfTabGraph(Composite parent, long nodeId, PerfTabDci dci, PerfTabGraphSettings settings, View view, VisibilityValidator validator)
	{
		super(parent, SWT.BORDER);
		this.nodeId = nodeId;
      this.view = view;
		this.validator = validator;
		this.settings = settings;
		items.add(dci);
      session = Registry.getSession();
		
		setLayout(new FillLayout());
		
		chart = ChartFactory.createLineChart(this, SWT.NONE);
		chart.setZoomEnabled(false);
		chart.setTitleVisible(true);
		chart.setChartTitle(settings.getRuntimeTitle());
		chart.setLegendVisible(settings.isShowLegendAlways());
		chart.setExtendedLegend(settings.isExtendedLegend());
		chart.setLogScaleEnabled(settings.isLogScaleEnabled());
      chart.setUseMultipliers(settings.isUseMultipliers());
		chart.setStacked(settings.isStacked());
      
      final Date from = new Date(System.currentTimeMillis() - settings.getTimeRangeMillis());
      final Date to = new Date(System.currentTimeMillis());
      chart.setTimeRange(from, to);
		
		GraphItemStyle style = new GraphItemStyle(settings.getType(), settings.getColorAsInt(), 2, settings.isInvertedValues() ? GraphItemStyle.INVERTED : 0);
		chart.setItemStyles(Arrays.asList(new GraphItemStyle[] { style }));
		if (!settings.isAutoScale())
      {
		   chart.setYAxisRange(settings.getMinYScaleValue(), settings.getMaxYScaleValue());
      }
		chart.addParameter(new GraphItem(nodeId, dci.getId(), DataOrigin.INTERNAL, DataType.INT32, "", settings.getRuntimeName(), "%s")); //$NON-NLS-1$ //$NON-NLS-2$
		
		addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (refreshController != null)
               refreshController.dispose();
         }
      });
		
		chart.addMouseListener(new MouseListener() {
         /* (non-Javadoc)
          * @see org.eclipse.swt.events.MouseListener#mouseUp(org.eclipse.swt.events.MouseEvent)
          */
         @Override
         public void mouseUp(MouseEvent e)
         {
         }
         
         /* (non-Javadoc)
          * @see org.eclipse.swt.events.MouseListener#mouseDown(org.eclipse.swt.events.MouseEvent)
          */
         @Override
         public void mouseDown(MouseEvent e)
         {
         }
         
         /* (non-Javadoc)
          * @see org.eclipse.swt.events.MouseListener#mouseDoubleClick(org.eclipse.swt.events.MouseEvent)
          */
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            openHistoryGraph();
         }
      });

      chart.getPlotAreaControl().addMouseListener(new MouseListener() {
         /* (non-Javadoc)
          * @see org.eclipse.swt.events.MouseListener#mouseUp(org.eclipse.swt.events.MouseEvent)
          */
         @Override
         public void mouseUp(MouseEvent e)
         {
         }
         
         /* (non-Javadoc)
          * @see org.eclipse.swt.events.MouseListener#mouseDown(org.eclipse.swt.events.MouseEvent)
          */
         @Override
         public void mouseDown(MouseEvent e)
         {
         }
         
         /* (non-Javadoc)
          * @see org.eclipse.swt.events.MouseListener#mouseDoubleClick(org.eclipse.swt.events.MouseEvent)
          */
         @Override
         public void mouseDoubleClick(MouseEvent e)
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
/*
      actionAdjustX = HistoricalGraphView.createAction(ChartActionType.ADJUST_X, this);
      actionAdjustY = HistoricalGraphView.createAction(ChartActionType.ADJUST_Y, this);
      actionAdjustBoth = HistoricalGraphView.createAction(ChartActionType.ADJUST_BOTH, this);

      presetActions = HistoricalGraphView.createPresetActions(new HistoricalGraphView.PresetHandler() {
         @Override
         public void onPresetSelected(int units, int range)
         {
            settings.setTimeUnits(units);
            settings.setTimeRange(range);
            refreshData();
         }
      });
      */
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
      
      Menu menu = manager.createContextMenu((Control)chart);
      ((Control)chart).setMenu(menu);
      for(Control ch : ((Composite)chart).getChildren())
      {
         ch.setMenu(menu);
      }
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
		chart.setLegendVisible(true);
		if (settings.isExtendedLegend())
		   chart.setExtendedLegend(true);
		synchronized(items)
		{
			items.add(dci);
			GraphItemStyle style = new GraphItemStyle(settings.getType(), settings.getColorAsInt(), 2, settings.isInvertedValues() ? GraphItemStyle.INVERTED : 0);
			List<GraphItemStyle> styles = new ArrayList<GraphItemStyle>(chart.getItemStyles());
			if (styles.size() < items.size())
				styles.add(style);
			else
				styles.set(items.size() - 1, style);
			chart.setItemStyles(styles);
			chart.addParameter(new GraphItem(nodeId, dci.getId(), DataOrigin.INTERNAL, DataType.INT32, "", settings.getRuntimeName(), "%s")); //$NON-NLS-1$ //$NON-NLS-2$
		}
	}
	
	/**
	 * Start chart update
	 */
	public void start()
	{
      refreshController = new ViewRefreshController(view, 30, new Runnable() {
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
		
      Job job = new Job(i18n.tr("Updating performance view"), view) {
			private PerfTabDci currentDci;

			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final Date from = new Date(System.currentTimeMillis() - settings.getTimeRangeMillis());
				final Date to = new Date(System.currentTimeMillis());
				synchronized(items)
				{
					final DciData[] data = new DciData[items.size()];
					for(int i = 0; i < data.length; i++)
					{
						currentDci = items.get(i);
						data[i] = session.getCollectedData(nodeId, currentDci.getId(), from, to, 0, HistoricalDataType.PROCESSED);
					}
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							if (!((Widget)chart).isDisposed())
							{
								chart.setTimeRange(from, to);
								for(int i = 0; i < data.length; i++)
									chart.updateParameter(i, data[i], true);
							}
							updateInProgress = false;
						}
					});
				}
			}

			@Override
			protected String getErrorMessage()
			{
            return String.format(i18n.tr("Cannot get value for DCI %s (%s)"), currentDci.getId(), currentDci.getDescription());
			}

			@Override
			protected void jobFailureHandler()
			{
				updateInProgress = false;
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!((Widget)chart).isDisposed())
                     chart.addError(getErrorMessage());
               }
            });
				super.jobFailureHandler();
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
      // TODO: implement graph open
	}

   /**
    * @see org.netxms.nxmc.modules.charts.api.HistoricalChartOwner#getChart()
    */
   @Override
   public HistoricalDataChart getChart()
   {
      return chart;
   }
}
