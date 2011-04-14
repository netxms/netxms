/**
 * 
 */
package org.netxms.ui.eclipse.googlemaps.views;

import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.googlemaps.tools.MapAccessor;
import org.netxms.ui.eclipse.googlemaps.widgets.GoogleMap;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Loaction map view
 * 
 */
public class LocationMap extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.googlemaps.view.location_map";
	public static final String JOB_FAMILY = "MapViewJob";
	
	private GoogleMap map;
	private MapAccessor mapAccessor;
	private GenericObject object;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		try
		{
			long id = Long.parseLong(site.getSecondaryId());
			object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(id);
		}
		catch(Exception e)
		{
			throw new PartInitException("Cannot initialize geolocation view: internal error", e);
		}
		if (object == null)
			throw new PartInitException("Cannot initialize geolocation view: object not found");
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		makeActions();
		contributeToActionBars();
		createPopupMenu();
		
		// Map control
		map = new GoogleMap(parent, SWT.BORDER);
		map.setSiteService((IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class));
		
		// Initial map view
		mapAccessor = new MapAccessor(object.getGeolocation());
		mapAccessor.setZoom(14);
		map.showMap(mapAccessor);
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
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
	}

	/**
	 * Create actions
	 */
	private void makeActions()
	{
	}

	/**
	 * Create pop-up menu for variable list
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
/*		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);*/
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr
	 *           Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		super.dispose();
	}
}
