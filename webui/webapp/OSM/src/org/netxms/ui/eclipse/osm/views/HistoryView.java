/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.osm.Messages;
import org.netxms.ui.eclipse.osm.dialogs.TimeSelectionDialog;
import org.netxms.ui.eclipse.osm.tools.MapAccessor;
import org.netxms.ui.eclipse.osm.widgets.GeoMapViewer;
import org.netxms.ui.eclipse.osm.widgets.helpers.GeoMapListener;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Base class for all geographical views
 */
public class HistoryView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.osm.views.HistoryView"; //$NON-NLS-1$
	public static final String JOB_FAMILY = "MapViewJob"; //$NON-NLS-1$
	private static final int[] presetRanges = { 10, 30, 60, 120, 240, 720, 1440, 2880, 7200, 10080, 44640, 525600 };
   private static final String[] presetNames = 
      { "10 minutes", "30 minutes", "1 hour", "2 hours", "4 hours", "12 hours", "Today",
        "Last 2 days", "Last 5 days", "This week", "This month","This Year" };

	
	protected GeoMapViewer map;
	
	private MapAccessor mapAccessor;
	private int zoomLevel = 15;
	private Action actionZoomIn;
	private Action actionZoomOut;
   private Action[] presetActions;
   private Action setConfigurableTime;
	private AbstractObject object;
	
	/**
	 * Get initial center point for displayed map
	 * 
	 * @return
	 */
	protected GeoLocation getInitialCenterPoint() 
	{
	   return object.getGeolocation();
	}
	
	/**
	 * Get initial zoom level
	 * 
	 * @return
	 */
	protected int getInitialZoomLevel()
	{
      return 15;
	}

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
			Platform.getAdapterManager().loadAdapter(((NXCSession)ConsoleSharedData.getSession()).getTopLevelObjects()[0], "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$
		}
		catch(Exception e)
		{
		}
		
		try
      {
         long id = Long.parseLong(site.getSecondaryId());
         object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(id);
         setPartName(Messages.get().LocationMap_PartNamePrefix + object.getObjectName());
      }
      catch(Exception e)
      {
         throw new PartInitException(Messages.get().LocationMap_InitError1, e);
      }
      if (object == null)
         throw new PartInitException(Messages.get().LocationMap_InitError2);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		// Map control
		map = new GeoMapViewer(parent, SWT.BORDER, true, object);
		map.setViewPart(this);
		
		createActions(parent);
		contributeToActionBars();
		createPopupMenu();
		
		// Initial map view
		mapAccessor = new MapAccessor(getInitialCenterPoint());
		zoomLevel = getInitialZoomLevel();
		mapAccessor.setZoom(zoomLevel);
		map.showMap(mapAccessor);
		
		map.addMapListener(new GeoMapListener() {
			@Override
			public void onZoom(int zoomLevel)
			{
				HistoryView.this.zoomLevel = zoomLevel;
				mapAccessor.setZoom(zoomLevel);
				actionZoomIn.setEnabled(zoomLevel < 18);
				actionZoomOut.setEnabled(zoomLevel > 0);
			}

			@Override
			public void onPan(GeoLocation centerPoint)
			{
				mapAccessor.setLatitude(centerPoint.getLatitude());
				mapAccessor.setLongitude(centerPoint.getLongitude());
			}
		});
	}

	/**
	 * Create actions
	 */
	protected void createActions(final Composite parent)
	{
		actionZoomIn = new Action(Messages.get().AbstractGeolocationView_ZoomIn) {
			@Override
			public void run()
			{
				setZoomLevel(zoomLevel + 1);
			}
		};
		actionZoomIn.setImageDescriptor(SharedIcons.ZOOM_IN);
	
		actionZoomOut = new Action(Messages.get().AbstractGeolocationView_ZoomOut) {
			@Override
			public void run()
			{
				setZoomLevel(zoomLevel - 1);
			}
		};
		actionZoomOut.setImageDescriptor(SharedIcons.ZOOM_OUT);
		
		presetActions = new Action[presetRanges.length];
      for(int i = 0; i < presetRanges.length; i++)
      {
         final Integer presetIndex = i;
         presetActions[i] = new Action(presetNames[i]) {
            @Override
            public void run()
            {
               map.changeTimePeriod(presetRanges[presetIndex]);
            }
         };
      }
      
      setConfigurableTime = new Action("Set time frame") 
      {
         @Override
         public void run()
         {
            TimeSelectionDialog dialog = new TimeSelectionDialog(parent.getShell());
            int result = dialog.open();
            if (result == Window.CANCEL)
               return;
            map.changeTimePeriod((int)dialog.getTimeInMinutes());
            if(dialog.getTimeFrameType() == GraphSettings.TIME_FRAME_FIXED)
            {
               map.changeTimePeriod(dialog.getTimeFrom());
            }
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
	 * @param manager Menu manager for pull-down menu
	 */
	protected void fillLocalPullDown(IMenuManager manager)
	{
	   MenuManager presets = new MenuManager("&Presets");
      for(int i = 0; i < presetActions.length; i++)
         presets.add(presetActions[i]);
      
      manager.add(presets);
      manager.add(new Separator()); 
      manager.add(setConfigurableTime);
      manager.add(new Separator());	   
		manager.add(actionZoomIn);
		manager.add(actionZoomOut);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager Menu manager for local toolbar
	 */
	protected void fillLocalToolBar(IToolBarManager manager)
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
		menuMgr.addMenuListener(new IMenuListener() {
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
	   MenuManager presets = new MenuManager("&Presets");
      for(int i = 0; i < presetActions.length; i++)
         presets.add(presetActions[i]);
      
      manager.add(presets);
      manager.add(new Separator()); 
      manager.add(setConfigurableTime);
      manager.add(new Separator());
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
	 * Set new zoom level for map
	 * 
	 * @param newLevel new zoom level
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

	/**
	 * @return the mapAccessor
	 */
	protected MapAccessor getMapAccessor()
	{
		return mapAccessor;
	}
}
