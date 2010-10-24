/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.views;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.api.DataChart;
import org.netxms.ui.eclipse.charts.widgets.LineChart;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.swtchart.IAxis;
import org.swtchart.ILineSeries;
import org.swtchart.LineStyle;

/**
 * History graph view
 *
 */
public class HistoryGraph extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.charts.views.HistoryGraph";
	public static final String PREDEFINED_GRAPH_SUBID = "org.netxms.ui.eclipse.charts.predefinedGraph";
	
	private static final int[] presetRanges = { 10, 30, 60, 120, 240, 1440, 10080, 44640, 525600 };
	private static final String[] presetNames = { "10 minutes", "30 minutes", "hour", "2 hours", "4 hours",
	                                              "day", "week", "month", "year" };

	private static final String KEY_AUTO_REFRESH = "autoRefresh";
	private static final String KEY_REFRESH_INTERVAL = "refreshInterval";
	private static final String KEY_LOG_SCALE = "logScale";
	private static final String KEY_SHOW_LEGEND = "showLegend";
	private static final String KEY_LEGEND_POSITION = "legendPosition";
	
	private NXCSession session;
	private ArrayList<GraphItem> items = new ArrayList<GraphItem>(1);
	private ArrayList<GraphItemStyle> itemStyles = new ArrayList<GraphItemStyle>(GraphSettings.MAX_GRAPH_ITEM_COUNT);
	private LineChart chart;
	private boolean updateInProgress = false;
	private Runnable refreshTimer;
	
	private Date timeFrom;
	private Date timeTo;
	private long timeRange = 3600000;	// 1 hour
	private boolean autoRefreshEnabled = true;
	private boolean useLogScale = false;
	private int autoRefreshInterval = 30000;	// 30 seconds
	private boolean showLegend = true;
	private int legendPosition = DataChart.POSITION_BOTTOM;
	
	private RefreshAction actionRefresh;
	private Action actionAutoRefresh;
	private Action actionZoomIn;
	private Action actionZoomOut;
	private Action actionAdjustX;
	private Action actionAdjustY;
	private Action actionAdjustBoth;
	private Action actionLogScale;
	private Action actionShowLegend;
	private Action actionLegendLeft;
	private Action actionLegendRight;
	private Action actionLegendTop;
	private Action actionLegendBottom;
	private Action[] presetActions;
	
	/**
	 * Default constructor
	 */
	public HistoryGraph()
	{
		// Create default item styles
		IPreferenceStore preferenceStore = Activator.getDefault().getPreferenceStore();
		for(int i = 0; i < GraphSettings.MAX_GRAPH_ITEM_COUNT; i++)
			itemStyles.add(new GraphItemStyle(GraphItemStyle.LINE, rgbToInt(PreferenceConverter.getColor(preferenceStore, "Chart.Colors.Data." + i)), 0, 0));
	}

	/* (non-Javadoc)
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
				if (chart.isDisposed())
					return;
				
				updateChart();
				display.timerExec(autoRefreshInterval, this);
			}
		};
		
		session = (NXCSession)ConsoleSharedData.getSession();
		
		timeFrom = new Date(System.currentTimeMillis() - timeRange);
		timeTo = new Date(System.currentTimeMillis());
		
		// Extract DCI ids from view id
		// (first field will be unique view id, so we skip it)
		String id = site.getSecondaryId();
		String[] fields = id.split("&");
		if (!fields[0].equals(PREDEFINED_GRAPH_SUBID))
		{
			for(int i = 1; i < fields.length; i++)
			{
				String[] subfields = fields[i].split("\\@");
				if (subfields.length == 6)
				{
					try
					{
						items.add(new GraphItem(
								Long.parseLong(subfields[0], 10),	// Node ID 
								Long.parseLong(subfields[1], 10),	// DCI ID
								Integer.parseInt(subfields[2], 10),	// source
								Integer.parseInt(subfields[3], 10),	// data type
								URLDecoder.decode(subfields[4], "UTF-8"),   // name
								URLDecoder.decode(subfields[5], "UTF-8"))); // description
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
			
			// Set view title to "host name: dci description" if
			// we have only one DCI
			if (items.size() == 1)
			{
				GraphItem item = items.get(0);
				GenericObject object = session.findObjectById(item.getNodeId());
				if (object != null)
				{
					setPartName(object.getObjectName() + ": " + item.getDescription());
				}
			}
		}
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
	 */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		init(site);
		
		if (memento != null)
		{
			autoRefreshEnabled = safeCast(memento.getBoolean(KEY_AUTO_REFRESH), autoRefreshEnabled);
			autoRefreshInterval = safeCast(memento.getInteger(KEY_REFRESH_INTERVAL), autoRefreshInterval);
			showLegend = safeCast(memento.getBoolean(KEY_SHOW_LEGEND), showLegend);
			legendPosition = safeCast(memento.getInteger(KEY_LEGEND_POSITION), legendPosition);
			useLogScale = safeCast(memento.getBoolean(KEY_LOG_SCALE), useLogScale);
		}
	}

	private static int safeCast(Integer i, int defval)
	{
		return (i != null) ? i : defval;
	}
	
	private static boolean safeCast(Boolean b, boolean defval)
	{
		return (b != null) ? b : defval;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#saveState(org.eclipse.ui.IMemento)
	 */
	@Override
	public void saveState(IMemento memento)
	{
		memento.putBoolean(KEY_AUTO_REFRESH, autoRefreshEnabled);
		memento.putInteger(KEY_REFRESH_INTERVAL, autoRefreshInterval);
		memento.putBoolean(KEY_SHOW_LEGEND, showLegend);
		memento.putInteger(KEY_LEGEND_POSITION, legendPosition);
		memento.putBoolean(KEY_LOG_SCALE, useLogScale);
	}
	
	/**
	 * Initialize this view with predefined graph settings
	 * 
	 * @param gs graph settings
	 */
	public void initPredefinedGraph(GraphSettings gs)
	{
		// General settings
		setPartName(gs.getTitle());
		chart.setChartTitle(gs.getTitle());

		// Chart visual settings
		itemStyles.clear();
		itemStyles.addAll(Arrays.asList(gs.getItemStyles()));
		chart.setLogScaleEnabled(gs.isLogScale());
		setGridVisible(gs.isGridVisible());
		chart.setLegendVisible(gs.isLegendVisible());
		
		// Data
		items.clear();
		items.addAll(Arrays.asList(gs.getItems()));
		
		getDataFromServer();

		// Automatic refresh
		autoRefreshInterval = gs.getAutoRefreshInterval();
		autoRefreshEnabled = gs.isAutoRefresh();
		actionAutoRefresh.setChecked(autoRefreshEnabled);
		HistoryGraph.this.getSite().getShell().getDisplay().timerExec(autoRefreshEnabled ? autoRefreshInterval : -1, refreshTimer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		chart = new LineChart(parent, SWT.NONE);
		
		setGridVisible(true);
		chart.setLogScaleEnabled(useLogScale);
		chart.setLegendVisible(showLegend);
		chart.setLegendPosition(legendPosition);
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		getDataFromServer();
		if (autoRefreshEnabled)
			getSite().getShell().getDisplay().timerExec(autoRefreshInterval, refreshTimer);
	}

	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(chart);
		chart.setMenu(menu);
		for(Control ch : chart.getChildren())
		{
			ch.setMenu(menu);
		}
	}

	/**
	 * Get DCI data from server
	 */
	private void getDataFromServer()
	{
		// Request data from server
		Job job = new Job("Get DCI values for history graph")
		{
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				final DciData[] data = new DciData[items.size()];
				try
				{
					for(int i = 0; i < items.size(); i++)
					{
						GraphItem item = items.get(i);
						data[i] = session.getCollectedData(item.getNodeId(), item.getDciId(), timeFrom, timeTo, 0);
					}
					status = Status.OK_STATUS;
	
					new UIJob("Update chart") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							chart.setTimeRange(timeFrom, timeTo);
							setChartData(data);
							updateInProgress = false;
							return Status.OK_STATUS;
						}
					}.schedule();
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
	                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
	                    "Cannot get DCI values for history graph: " + e.getMessage(), e);
					updateInProgress = false;
				}
				return status;
			}

			/* (non-Javadoc)
			 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
			 */
			@Override
			public boolean belongsTo(Object family)
			{
				return family == Activator.PLUGIN_ID;
			}
		};
		IWorkbenchSiteProgressService siteService =
	      (IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class);
		siteService.schedule(job, 0, true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		chart.setFocus();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				updateChart();
			}
		};
		
		actionAutoRefresh = new Action() {
			@Override
			public void run()
			{
				autoRefreshEnabled = !autoRefreshEnabled;
				setChecked(autoRefreshEnabled);
				HistoryGraph.this.getSite().getShell().getDisplay().timerExec(autoRefreshEnabled ? autoRefreshInterval : -1, refreshTimer);
			}
		};
		actionAutoRefresh.setText("Refresh &automatically");
		actionAutoRefresh.setChecked(autoRefreshEnabled);
		
		actionLogScale = new Action() {
			@Override
			public void run()
			{
				useLogScale = !useLogScale;
				setChecked(useLogScale);
				chart.setLogScaleEnabled(useLogScale);
				chart.redraw();
			}
		};
		actionLogScale.setText("&Logarithmic scale");
		actionLogScale.setChecked(useLogScale);
		
		actionZoomIn = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				chart.getAxisSet().zoomIn();
				chart.redraw();
			}
		};
		actionZoomIn.setText("Zoom &in");
		actionZoomIn.setImageDescriptor(Activator.getImageDescriptor("icons/zoom_in.png"));

		actionZoomOut = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				chart.getAxisSet().zoomOut();
				chart.redraw();
			}
		};
		actionZoomOut.setText("Zoom &out");
		actionZoomOut.setImageDescriptor(Activator.getImageDescriptor("icons/zoom_out.png"));

		actionAdjustX = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				for(IAxis axis : chart.getAxisSet().getXAxes())
				{
					axis.adjustRange();
				}
				chart.redraw();
			}
		};
		actionAdjustX.setText("Adjust &X axis");
		actionAdjustX.setImageDescriptor(Activator.getImageDescriptor("icons/adjust_x.png"));

		actionAdjustY = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				for(IAxis axis : chart.getAxisSet().getYAxes())
				{
					axis.adjustRange();
				}
				chart.redraw();
			}
		};
		actionAdjustY.setText("Adjust &Y axis");
		actionAdjustY.setImageDescriptor(Activator.getImageDescriptor("icons/adjust_y.png"));

		actionAdjustBoth = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				for(IAxis axis : chart.getAxisSet().getAxes())
				{
					axis.adjustRange();
				}
				chart.redraw();
			}
		};
		actionAdjustBoth.setText("&Adjust");
		actionAdjustBoth.setImageDescriptor(Activator.getImageDescriptor("icons/adjust.png"));

		actionShowLegend = new Action("&Show legend") {
			@Override
			public void run()
			{
				showLegend = !showLegend;
				setChecked(showLegend);
				chart.setLegendVisible(showLegend);
			}
		};
		actionShowLegend.setChecked(showLegend);
		
		actionLegendLeft = new Action("Place on &left", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = DataChart.POSITION_LEFT;
				chart.setLegendPosition(legendPosition);
			}
		};
		actionLegendLeft.setChecked(legendPosition == DataChart.POSITION_LEFT);
		
		actionLegendRight = new Action("Place on &right", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = DataChart.POSITION_RIGHT;
				chart.setLegendPosition(legendPosition);
			}
		};
		actionLegendRight.setChecked(legendPosition == DataChart.POSITION_RIGHT);
		
		actionLegendTop = new Action("Place on &top", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = DataChart.POSITION_TOP;
				chart.setLegendPosition(legendPosition);
			}
		};
		actionLegendTop.setChecked(legendPosition == DataChart.POSITION_LEFT);
		
		actionLegendBottom = new Action("Place on &bottom", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				legendPosition = DataChart.POSITION_BOTTOM;
				chart.setLegendPosition(legendPosition);
			}
		};
		actionLegendBottom.setChecked(legendPosition == DataChart.POSITION_LEFT);

		presetActions = new Action[presetRanges.length];
		for(int i = 0; i < presetRanges.length; i++)
		{
			final Integer presetIndex = i;
			presetActions[i] = new Action() {
				/* (non-Javadoc)
				 * @see org.eclipse.jface.action.Action#run()
				 */
				@Override
				public void run()
				{
					setTimeRange((long)presetRanges[presetIndex] * 60000L);
				}
			};
			presetActions[i].setText("Last " + presetNames[i]);
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
	 * @param manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		MenuManager presets = new MenuManager("&Presets");
		for(int i = 0; i < presetActions.length; i++)
			presets.add(presetActions[i]);
		
		MenuManager legend = new MenuManager("&Legend");
		legend.add(actionShowLegend);
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
		manager.add(actionLogScale);
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
		MenuManager presets = new MenuManager("&Presets");
		for(int i = 0; i < presetActions.length; i++)
			presets.add(presetActions[i]);

		MenuManager legend = new MenuManager("&Legend");
		legend.add(actionShowLegend);
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
		manager.add(actionLogScale);
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
		manager.add(actionAdjustBoth);
		manager.add(actionAdjustX);
		manager.add(actionAdjustY);
		manager.add(new Separator());
		manager.add(actionZoomIn);
		manager.add(actionZoomOut);
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
			addItemToChart(i, items.get(i), data[i]);
		
		chart.getAxisSet().getYAxis(0).adjustRange();
		chart.redraw();
	}
	
	/**
	 * Add single DCI to chart
	 * 
	 * @param data DCI data
	 */
	private void addItemToChart(int index, final GraphItem item, final DciData data)
	{
		final DciDataRow[] values = data.getValues();
		
		// Create series
		Date[] xSeries = new Date[values.length];
		double[] ySeries = new double[values.length];
		for(int i = 0; i < values.length; i++)
		{
			xSeries[i] = values[i].getTimestamp();
			ySeries[i] = values[i].getValueAsDouble();
		}
		
		ILineSeries series = chart.addLineSeries(index, item.getDescription(), xSeries, ySeries);
		applyItemStyle(index, series);
	}
	
	/**
	 * Update chart
	 */
	private void updateChart()
	{
		if (updateInProgress)
			return;
		
		updateInProgress = true;
		timeFrom = new Date(System.currentTimeMillis() - timeRange);
		timeTo = new Date(System.currentTimeMillis());
		getDataFromServer();
	}

	/**
	 * Set new time range
	 * @param range Offset from "now" in milliseconds
	 */
	private void setTimeRange(long range)
	{
		timeRange = range;
		updateChart();
	}
	
	/**
	 * Set chart grid visibility
	 * 
	 * @param visible true if grid must be visible
	 */
	protected void setGridVisible(boolean visible)
	{
		final LineStyle ls = visible ? LineStyle.DOT : LineStyle.NONE;
		chart.getAxisSet().getXAxis(0).getGrid().setStyle(ls);
		chart.getAxisSet().getYAxis(0).getGrid().setStyle(ls);
	}
	
	/**
	 * Apply graph item style
	 * 
	 * @param index item index
	 * @param series added series
	 */
	private void applyItemStyle(int index, ILineSeries series)
	{
		if ((index < 0) || (index >= itemStyles.size()))
			return;

		GraphItemStyle style = itemStyles.get(index);
		series.setLineColor(colorFromInt(style.getColor()));
	}
	
	/**
	 * Create Color object from integer RGB representation
	 * @param rgb color's rgb representation
	 * @return color object
	 */
	private Color colorFromInt(int rgb)
	{
		// All colors on server stored as BGR: red in less significant byte and blue in most significant byte
		return new Color(getSite().getShell().getDisplay(), rgb & 0xFF, (rgb >> 8) & 0xFF, rgb >> 16);
	}
	
	/**
	 * Create integer value from Red/Green/Blue
	 * 
	 * @param r red
	 * @param g green
	 * @param b blue
	 * @return
	 */
	private static int rgbToInt(RGB rgb)
	{
		return rgb.red | (rgb.green << 8) | (rgb.blue << 16);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		getSite().getShell().getDisplay().timerExec(-1, refreshTimer);
		super.dispose();
	}
}
