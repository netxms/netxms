/**
 * 
 */
package org.netxms.ui.eclipse.topology.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.WirelessStation;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.views.helpers.WirelessStationComparator;
import org.netxms.ui.eclipse.topology.views.helpers.WirelessStationLabelProvider;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * List of wireless stations
 */
public class WirelessStations extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.WirelessStations"; //$NON-NLS-1$
	
	public static final int COLUMN_MAC_ADDRESS = 0;
	public static final int COLUMN_IP_ADDRESS = 1;
	public static final int COLUMN_NODE_NAME = 2;
	public static final int COLUMN_ACCESS_POINT = 3;
	public static final int COLUMN_RADIO = 4;
	public static final int COLUMN_SSID = 5;
	
	private NXCSession session;
	private long rootObject;
	private SortableTableViewer viewer;
	private Action actionRefresh;
	private Action actionCopy;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		try
		{
			rootObject = Long.parseLong(site.getSecondaryId());
		}
		catch(NumberFormatException e)
		{
			rootObject = 0;
		}

		session = (NXCSession)ConsoleSharedData.getSession();
		setPartName("Wireless Stations - " + session.getObjectName(rootObject));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { "MAC Address", "IP Address", "Node", "AP", "Radio", "SSID" };
		final int[] widths = { 120, 90, 180, 180, 100, 100 };
		viewer = new SortableTableViewer(parent, names, widths, 1, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new WirelessStationLabelProvider());
		viewer.setComparator(new WirelessStationComparator());
		
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "WirelessStations");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "WirelessStations");
			}
		});

		createActions();
		contributeToActionBars();
		
		refresh();
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
				refresh();
			}
		};
	}

	/**
	 * Contribute actions to action bar
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
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRefresh);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/**
	 * Refresh content
	 */
	private void refresh()
	{
		new ConsoleJob("Get list of wireless stations", this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<WirelessStation> stations = session.getWirelessStations(rootObject);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(stations.toArray());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get list of wireless stations";
			}
		}.start();
	}
}
