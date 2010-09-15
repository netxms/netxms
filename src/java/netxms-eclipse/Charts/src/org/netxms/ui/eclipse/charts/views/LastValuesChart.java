package org.netxms.ui.eclipse.charts.views;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.ArrayList;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.RefreshAction;
import org.swtchart.Chart;

public abstract class LastValuesChart extends ViewPart
{

	protected Action actionAutoRefresh;

	protected RefreshAction actionRefresh;
	protected boolean autoRefreshEnabled = true;
	protected int autoRefreshInterval = 30000; // 30 seconds
	protected Chart chart; // Current chart
	protected ArrayList<GraphItem> items = new ArrayList<GraphItem>(8);
	protected Runnable refreshTimer;

	protected NXCSession session;
	private boolean updateInProgress = false;

	/**
	 * Add single DCI to chart
	 * 
	 * @param data
	 *           DCI data
	 */
	protected abstract void addItemToChart(int index, GraphItem item, double data, boolean padding);

	protected void contributeToActionBars()
	{
	}

	protected void createActions()
	{
		actionRefresh = new RefreshAction()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				updateChart();
			}
		};

		actionAutoRefresh = new Action()
		{
			@Override
			public void run()
			{
				autoRefreshEnabled = !autoRefreshEnabled;
				setChecked(autoRefreshEnabled);
				LastValuesChart.this.getSite().getShell().getDisplay().timerExec(
						autoRefreshEnabled ? autoRefreshInterval : -1, refreshTimer);
			}
		};
		actionAutoRefresh.setText("Refresh &automatically");
		actionAutoRefresh.setChecked(autoRefreshEnabled);
	}

	/**
	 * Create pop-up menu for user list
	 */
	protected void createPopupMenu()
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

	protected void fillContextMenu(IMenuManager manager)
	{
	}

	protected void fillData()
	{
		getDataFromServer();
		if (autoRefreshEnabled)
		{
			getSite().getShell().getDisplay().timerExec(autoRefreshInterval, refreshTimer);
		}
	}

	/**
	 * Get DCI data from server
	 */
	private void getDataFromServer()
	{
		ConsoleJob job = new ConsoleJob("Get DCI values for chart", this, Activator.PLUGIN_ID, Activator.PLUGIN_ID)
		{
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get DCI data";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final double[] values = new double[items.size()];
				for(int i = 0; i < items.size(); i++)
				{
					GraphItem item = items.get(i);
					DciData data = session.getCollectedData(item.getNodeId(), item.getDciId(), null, null, 1);
					DciDataRow value = data.getLastValue();
					values[i] = (value != null) ? value.getValueAsDouble() : 0.0;
				}

				new UIJob("Update chart")
				{
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						setChartData(values);
						return Status.OK_STATUS;
					}
				}.schedule();
			}
		};
		job.setUser(false);
		job.schedule();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		final Display display = site.getShell().getDisplay();
		refreshTimer = new Runnable()
		{
			@Override
			public void run()
			{
				if (chart.isDisposed())
					return;

				updateChart();
				display.timerExec(autoRefreshInterval, this);
			}
		};

		session = NXMCSharedData.getInstance().getSession();

		// Extract DCI ids from view id
		// (first field will be unique view id, so we skip it)
		String id = site.getSecondaryId();
		String[] fields = id.split("&");
		if (!fields[0].equals(HistoryGraph.PREDEFINED_GRAPH_SUBID))
		{
			for(int i = 1; i < fields.length; i++)
			{
				String[] subfields = fields[i].split("\\@");
				if (subfields.length == 6)
				{
					try
					{
						items.add(new GraphItem(Long.parseLong(subfields[0], 10), // Node ID 
								Long.parseLong(subfields[1], 10), // DCI ID
								Integer.parseInt(subfields[2], 10), // source
								Integer.parseInt(subfields[3], 10), // data type
								URLDecoder.decode(subfields[4], "UTF-8"), // name
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
		}
	}

	protected void initChart()
	{
		createActions();
		contributeToActionBars();
		createPopupMenu();
		fillData();
	}

	/**
	 * Set chart data
	 * 
	 * @param data
	 *           Retrieved DCI data
	 */
	private void setChartData(final double[] data)
	{
		for(int i = 0; i < data.length; i++)
			addItemToChart(i, items.get(i), data[i], i > 0);

		chart.getAxisSet().getYAxis(0).adjustRange();
		chart.redraw();
	}

	@Override
	public void setFocus()
	{
		chart.setFocus();
	}

	/**
	 * Update chart
	 */
	protected void updateChart()
	{
		if (updateInProgress)
			return;

		updateInProgress = true;
		getDataFromServer();
	}

}
