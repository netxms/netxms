/**
 * 
 */
package org.netxms.ui.eclipse.topology.views;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.RadioInterface;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.views.helpers.RadioInterfaceComparator;
import org.netxms.ui.eclipse.topology.views.helpers.RadioInterfaceLabelProvider;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * List of radio interfaces
 */
public class RadioInterfaces extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.RadioInterfaces"; //$NON-NLS-1$
	
	public static final int COLUMN_AP_NAME = 0;
	public static final int COLUMN_AP_MAC_ADDR = 1;
	public static final int COLUMN_AP_VENDOR = 2;
	public static final int COLUMN_AP_MODEL = 3;
	public static final int COLUMN_AP_SERIAL = 4;
	public static final int COLUMN_INDEX = 5;
	public static final int COLUMN_NAME = 6;
	public static final int COLUMN_MAC_ADDR = 7;
	public static final int COLUMN_CHANNEL = 8;
	public static final int COLUMN_TX_POWER_DBM = 9;
	public static final int COLUMN_TX_POWER_MW = 10;
	
	private NXCSession session;
	private long rootObject;
	private SortableTableViewer viewer;
	private Action actionRefresh;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;

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
		setPartName("Radio Interfaces - " + session.getObjectName(rootObject));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { "AP Name", "AP MAC Address", "AP Vendor", "AP Model", "AP Serial", "Radio Index", "Radio Name", "Radio MAC Address", "Channel", "Tx Power dBm", "Tx Power mW" };
		final int[] widths = { 120, 100, 140, 140, 100, 90, 120, 100, 90, 90, 90 };
		viewer = new SortableTableViewer(parent, names, widths, 1, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new RadioInterfaceLabelProvider());
		viewer.setComparator(new RadioInterfaceComparator());
		
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "RadioInterfaces");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "RadioInterfaces");
			}
		});

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
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

		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
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
		manager.add(actionExportAllToCsv);
		manager.add(new Separator());
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
		manager.add(actionExportAllToCsv);
		manager.add(actionRefresh);
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
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
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
		Node node = (Node)session.findObjectById(rootObject, Node.class);
		if (node == null)
			return;
		
		List<RadioInterface> list = new ArrayList<RadioInterface>();
		for(AbstractObject o : node.getAllChilds(AbstractObject.OBJECT_ACCESSPOINT))
		{
			if (o instanceof AccessPoint)
			{
				for(RadioInterface rif : ((AccessPoint)o).getRadios())
					list.add(rif);
			}
		}
		
		viewer.setInput(list.toArray());
	}
}
