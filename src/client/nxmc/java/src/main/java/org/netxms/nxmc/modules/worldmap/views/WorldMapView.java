/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.worldmap.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.worldmap.GeoMapListener;
import org.netxms.nxmc.modules.worldmap.tools.MapAccessor;
import org.netxms.nxmc.modules.worldmap.widgets.ObjectGeoLocationViewer;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * World map view
 */
public class WorldMapView extends View
{
   private final I18n i18n = LocalizationHelper.getI18n(WorldMapView.class);

   public static final String ID = "WorldMap";

   private ObjectGeoLocationViewer map;
   private MapAccessor mapAccessor;
   private PreferenceStore settings = PreferenceStore.getInstance();
   private int zoomLevel = 3;
   private Action actionCoordinates;
   private Action actionPlaceObject;
   private Action actionZoomIn;
   private Action actionZoomOut;

   /**
    * Create world map view
    */
   public WorldMapView()
   {
      super(LocalizationHelper.getI18n(WorldMapView.class).tr("World Map"), ResourceManager.getImageDescriptor("icons/worldmap.png"), ID, true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      map = new ObjectGeoLocationViewer(parent, SWT.NONE, this);

      createActions();
      createContextMenu();

      // Initial map view
      mapAccessor = new MapAccessor(new GeoLocation(settings.getAsDouble(ID + ".latitude", 0), settings.getAsDouble(ID + ".longitude", 0)));
      zoomLevel = settings.getAsInteger(ID + ".zoom", 3);
      mapAccessor.setZoom(zoomLevel);
      map.showMap(mapAccessor);

      map.addMapListener(new GeoMapListener() {
         @Override
         public void onZoom(int zoomLevel)
         {
            WorldMapView.this.zoomLevel = zoomLevel;
            mapAccessor.setZoom(zoomLevel);
            actionZoomIn.setEnabled(zoomLevel < MapAccessor.MAX_MAP_ZOOM);
            actionZoomOut.setEnabled(zoomLevel > MapAccessor.MIN_MAP_ZOOM);
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
   protected void createActions()
   {
      actionPlaceObject = new Action(i18n.tr("&Place object here..."), ResourceManager.getImageDescriptor("icons/worldmap/geo-pin.png")) {
         @Override
         public void run()
         {
            placeObject();
         }
      };

      actionZoomIn = new Action(i18n.tr("Zoom &in"), SharedIcons.ZOOM_IN) {
         @Override
         public void run()
         {
            setZoomLevel(zoomLevel + 1);
         }
      };

      actionZoomOut = new Action(i18n.tr("Zoom &out"), SharedIcons.ZOOM_OUT) {
         @Override
         public void run()
         {
            setZoomLevel(zoomLevel - 1);
         }
      };

      actionCoordinates = new Action("", SharedIcons.COPY_TO_CLIPBOARD) {
         @Override
         public void run()
         {
            WidgetHelper.copyToClipboard(map.getLocationAtPoint(map.getCurrentPoint()).toString());
         }
      };
   }

   /**
    * Create context menu
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new ObjectContextMenuManager(this, map, null) {
         @Override
         protected void fillContextMenu()
         {
            if (map.getSelection().isEmpty())
            {
               WorldMapView.this.fillContextMenu(this);
            }
            else
            {
               super.fillContextMenu();
            }
         }
      };
      Menu menu = menuMgr.createContextMenu(map);
      map.setMenu(menu);
   }

   /**
    * Fill context menu if selection is empty
    *
    * @param manager menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      actionCoordinates.setText(map.getLocationAtPoint(map.getCurrentPoint()).toString());
      manager.add(actionCoordinates);
      manager.add(new Separator());
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(new Separator());
      manager.add(actionPlaceObject);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      super.fillLocalToolBar(manager);
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      settings.set(ID + ".zoom", mapAccessor.getZoom());
      settings.set(ID + ".latitude", mapAccessor.getLatitude());
      settings.set(ID + ".longitude", mapAccessor.getLongitude());

      super.dispose();
   }

   /**
	 * Place object on map at mouse position
	 */
	private void placeObject()
	{
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(), ObjectSelectionDialog.createDataCollectionTargetSelectionFilter());
		if (dlg.open() == Window.OK)
		{
			final NXCObjectModificationData md = new NXCObjectModificationData(dlg.getSelectedObjects().get(0).getObjectId());
			md.setGeolocation(map.getLocationAtPoint(map.getCurrentPoint()));
         final NXCSession session = Registry.getSession();
         new Job(i18n.tr("Update object's geolocation"), this) {
				@Override
            protected void run(IProgressMonitor monitor) throws Exception
				{
					session.modifyObject(md);
				}

				@Override
				protected String getErrorMessage()
				{
               return i18n.tr("Cannot update object's geolocation");
				}
			}.start();
		}
	}
	
	/**
    * @see org.netxms.nxmc.base.views.View#onFilterModify()
    */
   @Override
	protected void onFilterModify()
	{
      map.setFilterString(getFilterText().trim().toLowerCase());
	   map.reloadMap();
	}

   /**
    * Set new zoom level for map
    * 
    * @param newLevel new zoom level
    */
   private void setZoomLevel(int newLevel)
   {
   	if ((newLevel < MapAccessor.MIN_MAP_ZOOM) || (newLevel > MapAccessor.MAX_MAP_ZOOM))
   		return;
   	
   	zoomLevel = newLevel;
   	mapAccessor.setZoom(zoomLevel);
   	map.showMap(mapAccessor);
   	
   	actionZoomIn.setEnabled(zoomLevel < MapAccessor.MAX_MAP_ZOOM);
   	actionZoomOut.setEnabled(zoomLevel > MapAccessor.MIN_MAP_ZOOM);
   }
}
