/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.StructuredSelection;
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
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.datacollection.GraphSettingsChangeListener;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.ChartFactory;
import org.netxms.ui.eclipse.charts.api.HistoricalDataChart;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.views.helpers.GraphSettingsFactory;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * History graph view
 */
public class HistoricalDataView extends ViewPart implements ISelectionProvider, GraphSettingsChangeListener
{
	public static final String ID = "org.netxms.ui.eclipse.perfview.views.HistoryGraph";
	public static final String PREDEFINED_GRAPH_SUBID = "org.netxms.ui.eclipse.charts.predefinedGraph";
	
	private static final int[] presetUnits = { GraphSettings.TIME_UNIT_MINUTE, GraphSettings.TIME_UNIT_MINUTE,
		GraphSettings.TIME_UNIT_HOUR, GraphSettings.TIME_UNIT_HOUR, GraphSettings.TIME_UNIT_HOUR,
		GraphSettings.TIME_UNIT_HOUR, GraphSettings.TIME_UNIT_DAY, GraphSettings.TIME_UNIT_DAY, 
		GraphSettings.TIME_UNIT_DAY, GraphSettings.TIME_UNIT_DAY, GraphSettings.TIME_UNIT_DAY, 
		GraphSettings.TIME_UNIT_DAY
	};
	private static final int[] presetRanges = { 10, 30, 1, 2, 4, 12, 1, 2, 5, 7, 31, 365 };
	private static final String[] presetNames = { "10 minutes", "30 minutes", "hour", "2 hours", "4 hours",
	                                              "12 hours", "day", "2 days", "5 days", "week", "month", "year" };

	private static final String KEY_AUTO_REFRESH = "autoRefresh";
	private static final String KEY_REFRESH_INTERVAL = "refreshInterval";
	private static final String KEY_LOG_SCALE = "logScale";
	private static final String KEY_SHOW_LEGEND = "showLegend";
	private static final String KEY_LEGEND_POSITION = "legendPosition";
	private static final String KEY_TITLE = "title";
	
	private NXCSession session;
	private ArrayList<GraphItem> items = new ArrayList<GraphItem>(1);
	private HistoricalDataChart chart;
	private boolean updateInProgress = false;
	private Runnable refreshTimer;
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
	
	private GraphSettings settings = GraphSettingsFactory.createDefault();
	
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
				if (((Widget)chart).isDisposed())
					return;
				
				updateChart();
				display.timerExec(settings.getAutoRefreshInterval(), this);
			}
		};
		
		session = (NXCSession)ConsoleSharedData.getSession();
		
		settings.setTimeFrom(new Date(System.currentTimeMillis() - settings.getTimeRangeMillis()));
		settings.setTimeTo(new Date(System.currentTimeMillis()));
		
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
			else if (items.size() > 1)
			{
				long nodeId = items.get(0).getNodeId();
				for(GraphItem item : items)
					if (item.getNodeId() != nodeId)
					{
						nodeId = -1;
						break;
					}
				if (nodeId != -1)
				{
					// All DCIs from same node, set title to "host name"
					GenericObject object = session.findObjectById(nodeId);
					if (object != null)
					{
						setPartName(object.getObjectName() + ": historical data");
					}
				}
			}
		}
		
		settings.setTitle(getPartName());
		settings.setItems(items.toArray(new GraphItem[items.size()]));
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
			settings.setAutoRefresh(safeCast(memento.getBoolean(KEY_AUTO_REFRESH), settings.isAutoRefresh()));
			settings.setAutoRefreshInterval(safeCast(memento.getInteger(KEY_REFRESH_INTERVAL), settings.getAutoRefreshInterval()));
			settings.setLegendVisible(safeCast(memento.getBoolean(KEY_SHOW_LEGEND), settings.isLegendVisible()));
			settings.setLegendPosition(safeCast(memento.getInteger(KEY_LEGEND_POSITION), settings.getLegendPosition()));
			settings.setLogScale(safeCast(memento.getBoolean(KEY_LOG_SCALE), settings.isLogScale()));
			settings.setTitle(memento.getString(KEY_TITLE));
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
		memento.putBoolean(KEY_AUTO_REFRESH, settings.isAutoRefresh());
		memento.putInteger(KEY_REFRESH_INTERVAL, settings.getAutoRefreshInterval());
		memento.putBoolean(KEY_SHOW_LEGEND, settings.isLegendVisible());
		memento.putInteger(KEY_LEGEND_POSITION, settings.getLegendPosition());
		memento.putBoolean(KEY_LOG_SCALE, settings.isLogScale());
		memento.putString(KEY_TITLE, settings.getTitle());
	}
	
	/**
	 * Initialize this view with predefined graph settings
	 * 
	 * @param gs graph settings
	 */
	public void initPredefinedGraph(GraphSettings gs)
	{
		settings = gs;
		settings.addChangeListener(this);
		configureGraphFromSettings();
	}
	
	/**
	 * Configure graph from graph settings
	 */
	private void configureGraphFromSettings()
	{
		// General settings
		setPartName(settings.getTitle());
		chart.setChartTitle(settings.getTitle());

		// Chart visual settings
		chart.setItemStyles(Arrays.asList(settings.getItemStyles()));
		chart.setLogScaleEnabled(settings.isLogScale());
		chart.setGridVisible(settings.isGridVisible());
		chart.setLegendVisible(settings.isLegendVisible());
		chart.setBackgroundColor(new ChartColor(settings.getBackgroundColor()));
		chart.setPlotAreaColor(new ChartColor(settings.getPlotBackgroundColor()));
		chart.setLegendColor(new ChartColor(settings.getLegendTextColor()), new ChartColor(settings.getLegendBackgroundColor()));
		chart.setAxisColor(new ChartColor(settings.getAxisColor()));
		chart.setGridColor(new ChartColor(settings.getGridColor()));
		
		// Data
		items.clear();
		items.addAll(Arrays.asList(settings.getItems()));

		for(GraphItem item : items)
			chart.addParameter(item);

		if (settings.getTimeFrameType() == GraphSettings.TIME_FRAME_BACK_FROM_NOW)
		{
			settings.setTimeFrom(new Date(System.currentTimeMillis() - settings.getTimeRangeMillis()));
			settings.setTimeTo(new Date(System.currentTimeMillis()));
		}
		
		getDataFromServer();

		// Automatic refresh
		actionAutoRefresh.setChecked(settings.isAutoRefresh());
		getSite().getShell().getDisplay().timerExec(settings.isAutoRefresh() ? settings.getAutoRefreshInterval() : -1, refreshTimer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		chart = ChartFactory.createLineChart(parent, SWT.NONE);
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
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
		menuMgr.addMenuListener(new IMenuListener()
		{
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
		// Request data from server
		ConsoleJob job = new ConsoleJob("Get DCI values for history graph", this, Activator.PLUGIN_ID, Activator.PLUGIN_ID)
		{
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				monitor.beginTask(getName(), items.size());
				final DciData[] data = new DciData[items.size()];
				final Threshold[][] thresholds = new Threshold[items.size()][];
				for(int i = 0; i < items.size(); i++)
				{
					GraphItem item = items.get(i);
					data[i] = session.getCollectedData(item.getNodeId(), item.getDciId(), settings.getTimeFrom(), settings.getTimeTo(), 0);
					thresholds[i] = session.getThresholds(item.getNodeId(), item.getDciId());
					monitor.worked(1);
				}

				new UIJob("Update chart") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						if (!((Widget)chart).isDisposed())
						{
							chart.setTimeRange(settings.getTimeFrom(), settings.getTimeTo());
							setChartData(data);
						}
						updateInProgress = false;
						return Status.OK_STATUS;
					}
				}.schedule();
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot get DCI values for history graph";
			}

			@Override
			protected void jobFailureHandler()
			{
				updateInProgress = false;
				super.jobFailureHandler();
			}
		};
		job.setUser(false);
		job.start();
	}

	/* (non-Javadoc)
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
				settings.setAutoRefresh(!settings.isAutoRefresh());
				setChecked(settings.isAutoRefresh());
				HistoricalDataView.this.getSite().getShell().getDisplay().timerExec(settings.isAutoRefresh() ? settings.getAutoRefreshInterval() : -1, refreshTimer);
			}
		};
		actionAutoRefresh.setText("Refresh &automatically");
		actionAutoRefresh.setChecked(settings.isAutoRefresh());
		
		actionLogScale = new Action() {
			@Override
			public void run()
			{
				try
				{
					chart.setLogScaleEnabled(!settings.isLogScale());
					settings.setLogScale(!settings.isLogScale());
				}
				catch(IllegalStateException e)
				{
					MessageDialog.openError(getSite().getShell(), "Error", "Cannot switch to logarithmic scale: " + e.getLocalizedMessage());
				}
				setChecked(settings.isLogScale());
			}
		};
		actionLogScale.setText("&Logarithmic scale");
		actionLogScale.setChecked(settings.isLogScale());
		
		actionZoomIn = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				chart.zoomIn();
			}
		};
		actionZoomIn.setText("Zoom &in");
		actionZoomIn.setImageDescriptor(SharedIcons.ZOOM_IN);

		actionZoomOut = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				chart.zoomOut();
			}
		};
		actionZoomOut.setText("Zoom &out");
		actionZoomOut.setImageDescriptor(SharedIcons.ZOOM_OUT);

		actionAdjustX = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				chart.adjustXAxis(true);
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
				chart.adjustYAxis(true);
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
				chart.adjustXAxis(false);
				chart.adjustYAxis(true);
			}
		};
		actionAdjustBoth.setText("&Adjust");
		actionAdjustBoth.setImageDescriptor(Activator.getImageDescriptor("icons/adjust.png"));

		actionShowLegend = new Action("&Show legend") {
			@Override
			public void run()
			{
				settings.setLegendVisible(!settings.isLegendVisible());
				setChecked(settings.isLegendVisible());
				chart.setLegendVisible(settings.isLegendVisible());
			}
		};
		actionShowLegend.setChecked(settings.isLegendVisible());
		
		actionLegendLeft = new Action("Place on &left", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				settings.setLegendPosition(GraphSettings.POSITION_LEFT);
				chart.setLegendPosition(settings.getLegendPosition());
			}
		};
		actionLegendLeft.setChecked(settings.getLegendPosition() == GraphSettings.POSITION_LEFT);
		
		actionLegendRight = new Action("Place on &right", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				settings.setLegendPosition(GraphSettings.POSITION_RIGHT);
				chart.setLegendPosition(settings.getLegendPosition());
			}
		};
		actionLegendRight.setChecked(settings.getLegendPosition() == GraphSettings.POSITION_RIGHT);
		
		actionLegendTop = new Action("Place on &top", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				settings.setLegendPosition(GraphSettings.POSITION_TOP);
				chart.setLegendPosition(settings.getLegendPosition());
			}
		};
		actionLegendTop.setChecked(settings.getLegendPosition() == GraphSettings.POSITION_TOP);
		
		actionLegendBottom = new Action("Place on &bottom", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				settings.setLegendPosition(GraphSettings.POSITION_BOTTOM);
				chart.setLegendPosition(settings.getLegendPosition());
			}
		};
		actionLegendBottom.setChecked(settings.getLegendPosition() == GraphSettings.POSITION_BOTTOM);

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
					settings.setTimeUnit(presetUnits[presetIndex]);
					settings.setTimeFrame(presetRanges[presetIndex]);
					updateChart();
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
		manager.add(new Separator());
		manager.add(new PropertyDialogAction(getSite(), this));
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
		settings.setTimeFrom(new Date(System.currentTimeMillis() - settings.getTimeRangeMillis()));
		settings.setTimeTo(new Date(System.currentTimeMillis()));
		getDataFromServer();
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
	 */
	@Override
	public void addSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.add(listener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
	 */
	@Override
	public ISelection getSelection()
	{
		return new StructuredSelection(settings);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
	 */
	@Override
	public void removeSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.remove(listener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void setSelection(ISelection selection)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.datacollection.GraphSettingsChangeListener#onGraphSettingsChange(org.netxms.client.datacollection.GraphSettings)
	 */
	@Override
	public void onGraphSettingsChange(GraphSettings settings)
	{
		if (this.settings == settings)
			configureGraphFromSettings();
	}
}
