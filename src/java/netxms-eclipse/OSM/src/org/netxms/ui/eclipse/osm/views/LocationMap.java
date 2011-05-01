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
package org.netxms.ui.eclipse.osm.views;

import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.osm.tools.MapAccessor;
import org.netxms.ui.eclipse.osm.widgets.GeoMapViewer;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Geolocation view
 */
public class LocationMap extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.osm.views.LocationMap";
	public static final String JOB_FAMILY = "MapViewJob";
	
	private GeoMapViewer map;
	private MapAccessor mapAccessor;
	private GenericObject object;
	private int zoomLevel = 15;
	
	private Action actionZoomIn;
	private Action actionZoomOut;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		// Initiate loading of required plugins if they was not loaded yet
		try
		{
			Platform.getAdapterManager().loadAdapter(((NXCSession)ConsoleSharedData.getSession()).getTopLevelObjects()[0], "org.eclipse.ui.model.IWorkbenchAdapter");
		}
		catch(Exception e)
		{
		}
		
		try
		{
			long id = Long.parseLong(site.getSecondaryId());
			object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(id);
			setPartName("Geolocation - " + object.getObjectName());
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
		// Map control
		map = new GeoMapViewer(parent, SWT.BORDER);
		map.setSiteService((IWorkbenchSiteProgressService)getSite().getAdapter(IWorkbenchSiteProgressService.class));
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		// Initial map view
		mapAccessor = new MapAccessor(object.getGeolocation());
		mapAccessor.setZoom(zoomLevel);
		map.showMap(mapAccessor);
		map.setCenterMarker(object);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionZoomIn = new Action("Zoom &in") {
			@Override
			public void run()
			{
				setZoomLevel(zoomLevel + 1);
			}
		};
		actionZoomIn.setImageDescriptor(SharedIcons.ZOOM_IN);

		actionZoomOut = new Action("Zoom &out") {
			@Override
			public void run()
			{
				setZoomLevel(zoomLevel - 1);
			}
		};
		actionZoomOut.setImageDescriptor(SharedIcons.ZOOM_OUT);
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
		manager.add(actionZoomIn);
		manager.add(actionZoomOut);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionZoomIn);
		manager.add(actionZoomOut);
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
		Menu menu = menuMgr.createContextMenu(map);
		map.setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param manager
	 *           Menu manager
	 */
	protected void fillContextMenu(final IMenuManager manager)
	{
		manager.add(actionZoomIn);
		manager.add(actionZoomOut);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		map.setFocus();
	}
	
	/**
	 * @param newLevel
	 */
	private void setZoomLevel(int newLevel)
	{
		if ((newLevel < 0) || (newLevel > 18))
			return;
		
		zoomLevel = newLevel;
		mapAccessor.setZoom(zoomLevel);
		map.showMap(mapAccessor);
		
		actionZoomIn.setEnabled(zoomLevel < 18);
		actionZoomOut.setEnabled(zoomLevel > 0);
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
