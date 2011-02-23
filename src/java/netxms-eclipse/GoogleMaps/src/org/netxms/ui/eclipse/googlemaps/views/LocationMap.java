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
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.netxms.ui.eclipse.googlemaps.tools.MapAccessor;
import org.netxms.ui.eclipse.googlemaps.widgets.GoogleMap;

/**
 * @author Victor
 * 
 */
public class LocationMap extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.googlemaps.view.location_map";
	public static final String JOB_FAMILY = "MapViewJob";
	
	private GoogleMap map;
	private MapAccessor mapAccessor;
	private Label infoText;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		makeActions();
		contributeToActionBars();
		createPopupMenu();
		
		GridLayout layout = new GridLayout();
		layout.horizontalSpacing = 0;
		layout.verticalSpacing = 0;
		layout.numColumns = 2;
		parent.setLayout(layout);
		
		// Map control
		map = new GoogleMap(parent, SWT.BORDER);
		map.setSiteService((IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class));
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		map.setLayoutData(gd);
		
		// Legend
		Composite legend = new Composite(parent, SWT.BORDER);
		GridLayout legendLayout = new GridLayout();
		legendLayout.horizontalSpacing = 0;
		legendLayout.verticalSpacing = 0;
		legend.setLayout(legendLayout);
		gd = new GridData();
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		legend.setLayoutData(gd);
		
		infoText = new Label(legend, SWT.WRAP);
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		infoText.setLayoutData(gd);

		// Initial map view
		mapAccessor = new MapAccessor(57.0, 24.0);
		mapAccessor.setZoom(13);
		map.showMap(mapAccessor);
		
		infoText.setText("Map centered at " + mapAccessor.getLatitude() + " " + mapAccessor.getLongitude());
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
