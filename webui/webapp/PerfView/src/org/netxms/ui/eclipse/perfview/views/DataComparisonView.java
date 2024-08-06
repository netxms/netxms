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
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.charts.api.ChartType;
import org.netxms.ui.eclipse.charts.widgets.Chart;
import org.netxms.ui.eclipse.compatibility.GraphItem;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ViewRefreshController;

/**
 * View for comparing DCI values visually using charts.
 */
public class DataComparisonView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.perfview.views.DataComparisionView"; //$NON-NLS-1$
	
	private static final String KEY_CHART_TYPE = "chartType"; //$NON-NLS-1$
	private static final String KEY_AUTO_REFRESH = "autoRefresh"; //$NON-NLS-1$
	private static final String KEY_REFRESH_INTERVAL = "autoRefreshInterval"; //$NON-NLS-1$
	private static final String KEY_LOG_SCALE = "logScale"; //$NON-NLS-1$
	private static final String KEY_TRANSPOSED = "isTransposed"; //$NON-NLS-1$
	private static final String KEY_TRANSLUCENT = "isTRanslucent"; //$NON-NLS-1$
	private static final String KEY_SHOW_LEGEND = "showLegend"; //$NON-NLS-1$
	private static final String KEY_LEGEND_POSITION = "legendPosition"; //$NON-NLS-1$

   private Chart chart;
	protected NXCSession session;
	private boolean updateInProgress = false;
	protected ArrayList<GraphItem> items = new ArrayList<GraphItem>(8);
	private ViewRefreshController refreshController;
	private boolean autoRefreshEnabled = true;
	private boolean useLogScale = false;
	private int autoRefreshInterval = 30;	// 30 seconds
   private ChartType chartType = ChartType.BAR;
	private boolean transposed = false;
	private boolean showLegend = true;
	private int legendPosition = ChartConfiguration.POSITION_BOTTOM;
	private boolean translucent = false;
   private Image[] titleImages = new Image[3];

	private RefreshAction actionRefresh;
	private Action actionAutoRefresh;
	private Action actionShowBarChart;
	private Action actionShowPieChart;
	private Action actionShowTranslucent;
	private Action actionUseLogScale;
	private Action actionHorizontal;
	private Action actionVertical;
	private Action actionShowLegend;
	private Action actionLegendLeft;
	private Action actionLegendRight;
	private Action actionLegendTop;
	private Action actionLegendBottom;
	private Action actionCopyImage;
	private Action actionSaveAsImage;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

      session = ConsoleSharedData.getSession();

      titleImages[0] = Activator.getImageDescriptor("icons/chart-bar-vertical.png").createImage(); // $NON-NLS-1$
      titleImages[1] = Activator.getImageDescriptor("icons/chart-pie.png").createImage(); // $NON-NLS-1$
      titleImages[2] = Activator.getImageDescriptor("icons/chart-scatter.png").createImage(); // $NON-NLS-1$

		// Extract information from view id
		//   first field is unique ID
		//   second is initial chart type
		//   third is DCI list
		String id = site.getSecondaryId();
		String[] fields = id.split("&"); //$NON-NLS-1$
		if (!fields[0].equals(HistoricalGraphView.PREDEFINED_GRAPH_SUBID))
		{
			try
			{
            chartType = ChartType.getByValue(Integer.parseInt(fields[1]));
			}
			catch(NumberFormatException e)
			{
            chartType = ChartType.BAR;
			}
			for(int i = 2; i < fields.length; i++)
			{
				String[] subfields = fields[i].split("\\@"); //$NON-NLS-1$
				if (subfields.length == 6)
				{
					try
					{
						items.add(new GraphItem(Long.parseLong(subfields[0], 10), // Node ID 
								Long.parseLong(subfields[1], 10), // DCI ID
								URLDecoder.decode(subfields[4], "UTF-8"), // name //$NON-NLS-1$
								URLDecoder.decode(subfields[5], "UTF-8"), // description //$NON-NLS-1$
								null, ChartDciConfig.DEFAULT, -1)); // format //$NON-NLS-1$
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
				else if (subfields.length == 8)
				{
					try
					{
						items.add(new GraphItem(Long.parseLong(subfields[0], 10), // Node ID 
								Long.parseLong(subfields[1], 10), // DCI ID
								URLDecoder.decode(subfields[4], "UTF-8"), // name //$NON-NLS-1$
								URLDecoder.decode(subfields[5], "UTF-8"), //$NON-NLS-1$
								URLDecoder.decode(subfields[6], "UTF-8"), //$NON-NLS-1$
								URLDecoder.decode(subfields[7], "UTF-8"), // description //$NON-NLS-1$
								null));  // format //$NON-NLS-1$
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
		}
		
      setTitleImage(getIconByChartType(chartType));
	}

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
    */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		init(site);
		
		if (memento != null)
		{
         chartType = ChartType.getByValue(safeCast(memento.getInteger(KEY_CHART_TYPE), chartType.getValue()));
			autoRefreshEnabled = safeCast(memento.getBoolean(KEY_AUTO_REFRESH), autoRefreshEnabled);
			autoRefreshInterval = safeCast(memento.getInteger(KEY_REFRESH_INTERVAL), autoRefreshInterval);
			translucent = safeCast(memento.getBoolean(KEY_TRANSLUCENT), translucent);
			transposed = safeCast(memento.getBoolean(KEY_TRANSPOSED), transposed);
			showLegend = safeCast(memento.getBoolean(KEY_SHOW_LEGEND), showLegend);
			legendPosition = safeCast(memento.getInteger(KEY_LEGEND_POSITION), legendPosition);
			useLogScale = safeCast(memento.getBoolean(KEY_LOG_SCALE), useLogScale);
         setTitleImage(getIconByChartType(chartType));
		}
	}
	
	/**
	 * @param i
	 * @param defval
	 * @return
	 */
	private static int safeCast(Integer i, int defval)
	{
		return (i != null) ? i : defval;
	}
	
	/**
	 * @param b
	 * @param defval
	 * @return
	 */
	private static boolean safeCast(Boolean b, boolean defval)
	{
		return (b != null) ? b : defval;
	}

   /**
    * @see org.eclipse.ui.part.ViewPart#saveState(org.eclipse.ui.IMemento)
    */
	@Override
	public void saveState(IMemento memento)
	{
      memento.putInteger(KEY_CHART_TYPE, chartType.getValue());
		memento.putBoolean(KEY_AUTO_REFRESH, autoRefreshEnabled);
		memento.putInteger(KEY_REFRESH_INTERVAL, autoRefreshInterval);
		memento.putBoolean(KEY_TRANSLUCENT, translucent);
		memento.putBoolean(KEY_TRANSPOSED, transposed);
		memento.putBoolean(KEY_SHOW_LEGEND, showLegend);
		memento.putInteger(KEY_LEGEND_POSITION, legendPosition);
		memento.putBoolean(KEY_LOG_SCALE, useLogScale);
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
	{
      ChartConfiguration chartConfiguration = new ChartConfiguration();
      chartConfiguration.setLegendPosition(legendPosition);
      chartConfiguration.setLegendVisible(showLegend);
      chartConfiguration.setTransposed(transposed);
      chartConfiguration.setTranslucent(translucent);

      chart = new Chart(parent, SWT.NONE, chartType, chartConfiguration);
		for(GraphItem item : items)
         chart.addParameter(item);
      chart.rebuild();

      createActions();
		contributeToActionBars();
		createPopupMenu();

		updateChart();

		refreshController = new ViewRefreshController(this, autoRefreshEnabled ? autoRefreshInterval : -1, new Runnable() {
			@Override
			public void run()
			{
				if (((Widget)chart).isDisposed())
					return;

				updateChart();
			}
		});
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
	@Override
	public void setFocus()
	{
      chart.setFocus();
	}

	/**
	 * Create pop-up menu for user list
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

		actionAutoRefresh = new Action(Messages.get().DataComparisonView_AutoRefresh) {
			@Override
			public void run()
			{
				autoRefreshEnabled = !autoRefreshEnabled;
				setChecked(autoRefreshEnabled);
				refreshController.setInterval(autoRefreshEnabled ? autoRefreshInterval : -1);
			}
		};
		actionAutoRefresh.setChecked(autoRefreshEnabled);
		
		actionUseLogScale = new Action(Messages.get().DataComparisonView_LogScale) {
			@Override
			public void run()
			{
				useLogScale = !useLogScale;
				setChecked(useLogScale);
            chart.getConfiguration().setLogScale(useLogScale);
            chart.rebuild();
			}
		};
		actionUseLogScale.setChecked(useLogScale);
		
		actionShowTranslucent = new Action(Messages.get().DataComparisonView_Translucent) {
			@Override
			public void run()
			{
				translucent = !translucent;
				setChecked(translucent);
            chart.getConfiguration().setTranslucent(translucent);
            chart.rebuild();
			}
		};
		actionShowTranslucent.setChecked(translucent);
		
		actionShowLegend = new Action(Messages.get().DataComparisonView_ShowLegend) {
			@Override
			public void run()
			{
				showLegend = !showLegend;
				setChecked(showLegend);
            chart.getConfiguration().setLegendVisible(showLegend);
            chart.rebuild();
			}
		};
		actionShowLegend.setChecked(showLegend);
		
		actionLegendLeft = new Action(Messages.get().DataComparisonView_PlaceOnLeft, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = ChartConfiguration.POSITION_LEFT;
            chart.getConfiguration().setLegendPosition(legendPosition);
            chart.rebuild();
			}
		};
		actionLegendLeft.setChecked(legendPosition == ChartConfiguration.POSITION_LEFT);
		
		actionLegendRight = new Action(Messages.get().DataComparisonView_PlaceOnRight, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = ChartConfiguration.POSITION_RIGHT;
            chart.getConfiguration().setLegendPosition(legendPosition);
            chart.rebuild();
			}
		};
		actionLegendRight.setChecked(legendPosition == ChartConfiguration.POSITION_RIGHT);
		
		actionLegendTop = new Action(Messages.get().DataComparisonView_PlaceOnTop, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = ChartConfiguration.POSITION_TOP;
            chart.getConfiguration().setLegendPosition(legendPosition);
            chart.rebuild();
			}
		};
		actionLegendTop.setChecked(legendPosition == ChartConfiguration.POSITION_LEFT);
		
		actionLegendBottom = new Action(Messages.get().DataComparisonView_PlaceOnBottom, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = ChartConfiguration.POSITION_BOTTOM;
            chart.getConfiguration().setLegendPosition(legendPosition);
            chart.rebuild();
			}
		};
		actionLegendBottom.setChecked(legendPosition == ChartConfiguration.POSITION_LEFT);
		
		actionShowBarChart = new Action(Messages.get().DataComparisonView_BarChart, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
            setChartType(ChartType.BAR);
			}
		};
      actionShowBarChart.setChecked(chartType == ChartType.BAR);
		actionShowBarChart.setImageDescriptor(Activator.getImageDescriptor("icons/chart-bar-vertical.png")); //$NON-NLS-1$
		
		actionShowPieChart = new Action(Messages.get().DataComparisonView_PieChart, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
            setChartType(ChartType.PIE);
			}
		};
      actionShowPieChart.setChecked(chartType == ChartType.PIE);
		actionShowPieChart.setImageDescriptor(Activator.getImageDescriptor("icons/chart-pie.png")); //$NON-NLS-1$

		actionHorizontal = new Action(Messages.get().DataComparisonView_ShowHorizontally, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				transposed = true;
            chart.getConfiguration().setTransposed(true);
            chart.rebuild();
			}
		};
		actionHorizontal.setChecked(transposed);
		actionHorizontal.setEnabled(chart.hasAxes());
		actionHorizontal.setImageDescriptor(Activator.getImageDescriptor("icons/chart-bar-horizontal.png")); //$NON-NLS-1$
		
		actionVertical = new Action(Messages.get().DataComparisonView_ShowVertically, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				transposed = false;
            chart.getConfiguration().setTransposed(false);
            chart.rebuild();
			}
		};
		actionVertical.setChecked(!transposed);
		actionVertical.setEnabled(chart.hasAxes());
		actionVertical.setImageDescriptor(Activator.getImageDescriptor("icons/chart-bar-vertical.png")); //$NON-NLS-1$

      actionCopyImage = new Action(Messages.get().DataComparisonView_CopyToClipboard, SharedIcons.COPY) {
         @Override
         public void run()
         {
            chart.copyToClipboard();
         }
      };

		actionSaveAsImage = new Action("Save as image", SharedIcons.SAVE_AS_IMAGE) {
			@Override
			public void run()
			{
            chart.saveAsImage(getSite().getShell());
			}
		};
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
	 * @param manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		MenuManager legend = new MenuManager(Messages.get().DataComparisonView_Legend);
		legend.add(actionShowLegend);
		legend.add(new Separator());
		legend.add(actionLegendLeft);
		legend.add(actionLegendRight);
		legend.add(actionLegendTop);
		legend.add(actionLegendBottom);
		
		manager.add(actionShowBarChart);
		manager.add(actionShowPieChart);
		manager.add(new Separator());
		manager.add(actionVertical);
		manager.add(actionHorizontal);
		manager.add(new Separator());
		manager.add(actionShowTranslucent);
		manager.add(actionUseLogScale);
		manager.add(actionAutoRefresh);
		manager.add(legend);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill context menu
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
		MenuManager legend = new MenuManager(Messages.get().DataComparisonView_Legend);
		legend.add(actionShowLegend);
		legend.add(new Separator());
		legend.add(actionLegendLeft);
		legend.add(actionLegendRight);
		legend.add(actionLegendTop);
		legend.add(actionLegendBottom);
		
		manager.add(actionShowBarChart);
		manager.add(actionShowPieChart);
		manager.add(new Separator());
		manager.add(actionVertical);
		manager.add(actionHorizontal);
		manager.add(new Separator());
		manager.add(actionShowTranslucent);
		manager.add(actionUseLogScale);
		manager.add(actionAutoRefresh);
		manager.add(legend);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionShowBarChart);
		manager.add(actionShowPieChart);
		manager.add(new Separator());
		manager.add(actionVertical);
		manager.add(actionHorizontal);
		manager.add(new Separator());
      manager.add(actionCopyImage);
		manager.add(actionSaveAsImage);		
		manager.add(new Separator());
		manager.add(actionRefresh);
	}
	
	/**
	 * Set new chart type
	 * @param newType
	 */
   private void setChartType(ChartType newType)
	{
		chartType = newType;
      chart.getConfiguration().setLabelsVisible(chartType == ChartType.PIE);
      chart.setType(chartType);
      chart.rebuild();

		actionHorizontal.setEnabled(chart.hasAxes());
		actionVertical.setEnabled(chart.hasAxes());
		try
		{
         setTitleImage(getIconByChartType(chartType));
			firePropertyChange(IWorkbenchPart.PROP_TITLE);
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
		}
	}

	/**
	 * Get DCI data from server
	 */
	private void updateChart()
	{
		if (updateInProgress)
			return;

		updateInProgress = true;
		ConsoleJob job = new ConsoleJob(Messages.get().DataComparisonView_JobName, this, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().DataComparisonView_JobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final double[] values = new double[items.size()];
				for(int i = 0; i < items.size(); i++)
				{
					GraphItem item = items.get(i);
					DciData data = (item.getType() == DataCollectionObject.DCO_TYPE_ITEM) ? 
							session.getCollectedData(item.getNodeId(), item.getDciId(), null, null, 1, HistoricalDataType.PROCESSED) :
							session.getCollectedTableData(item.getNodeId(), item.getDciId(), item.getInstance(), item.getDataColumn(), null, null, 1);
					DciDataRow value = data.getLastValue();
					values[i] = (value != null) ? value.getValueAsDouble() : 0.0;
				}

				final Threshold[][] thresholds = new Threshold[items.size()][];
            if ((chartType == ChartType.DIAL_GAUGE) || (chartType == ChartType.BAR_GAUGE) || (chartType == ChartType.CIRCULAR_GAUGE) || (chartType == ChartType.TEXT_GAUGE))
				{
					for(int i = 0; i < items.size(); i++)
					{
						GraphItem item = items.get(i);
						thresholds[i] = session.getThresholds(item.getNodeId(), item.getDciId());
					}
				}

				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
                  if ((chartType == ChartType.DIAL_GAUGE) || (chartType == ChartType.BAR_GAUGE) || (chartType == ChartType.CIRCULAR_GAUGE) || (chartType == ChartType.TEXT_GAUGE))
							for(int i = 0; i < thresholds.length; i++)
								chart.updateParameterThresholds(i, thresholds[i]);
						setChartData(values);
						chart.clearErrors();
						updateInProgress = false;
					}
				});
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
	 * Set new data for chart items
	 * 
	 * @param values new values
	 */
	private void setChartData(double[] values)
	{
		for(int i = 0; i < values.length; i++)
			chart.updateParameter(i, values[i], false);
		chart.refresh();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
	   refreshController.dispose();
		for(Image i : titleImages)
		{
			if (i != null)
				i.dispose();
		}
		super.dispose();
	}

   /**
    * Get icon for given chart type
    *
    * @param chartType
    * @return
    */
   private Image getIconByChartType(ChartType chartType)
   {
      switch(chartType)
      {
         case BAR:
            return titleImages[0];
         case PIE:
            return titleImages[1];
         default:
            return titleImages[2];
      }
   }
}
