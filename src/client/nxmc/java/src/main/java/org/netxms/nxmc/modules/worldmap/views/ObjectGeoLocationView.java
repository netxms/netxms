/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.worldmap.GeoMapListener;
import org.netxms.nxmc.modules.worldmap.tools.MapAccessor;
import org.netxms.nxmc.modules.worldmap.widgets.ObjectGeoLocationViewer;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Geolocation view
 */
public class ObjectGeoLocationView extends ObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectGeoLocationView.class);

   private ObjectGeoLocationViewer map;
   private MapAccessor mapAccessor;
   private PreferenceStore settings = PreferenceStore.getInstance();
   private int zoomLevel = 3;
   private boolean showPending = false;
   private boolean hideOtherObjects = false;
   private Action actionCoordinates;
   private Action actionZoomIn;
   private Action actionZoomOut;
   private Action actionHideOtherObjects;
   private Action actionUpdateObjectLocation;

   /**
    * @param name
    * @param image
    * @param id
    * @param hasFilter
    */
   public ObjectGeoLocationView()
   {
      super(LocalizationHelper.getI18n(ObjectGeoLocationView.class).tr("Geolocation"), ResourceManager.getImageDescriptor("icons/object-views/geolocation.png"), "GeoLocation", false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return super.isValidForContext(context) && (((AbstractObject)context).getGeolocation().getType() != GeoLocation.UNSET);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 300;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      GeoLocation l = (object != null) ? object.getGeolocation() : new GeoLocation(false);
      if (l.getType() != GeoLocation.UNSET)
      {
         mapAccessor.setLatitude(l.getLatitude());
         mapAccessor.setLongitude(l.getLongitude());
      }
      else
      {
         mapAccessor.setLatitude(0);
         mapAccessor.setLongitude(0);
      }

      if (hideOtherObjects)
         map.setRootObjectId(object.getObjectId());

      if (isVisible())
         map.showMap(mapAccessor);
      else
         showPending = true;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      if (showPending)
      {
         map.showMap(mapAccessor);
         showPending = false;
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      map = new ObjectGeoLocationViewer(parent, SWT.NONE, this);
      hideOtherObjects = settings.getAsBoolean(getBaseId() + ".hideOtherObjects", hideOtherObjects);
      if (hideOtherObjects)
      {
         map.setSingleObjectMode(true);
         map.setRootObjectId(getObjectId());
      }

      createActions();
      createPopupMenu();

      // Initial map view
      mapAccessor = new MapAccessor((getObject() != null) ? getObject().getGeolocation() : new GeoLocation(false));
      zoomLevel = settings.getAsInteger(getBaseId() + ".zoom", 15);
      mapAccessor.setZoom(zoomLevel);
      if (isVisible())
         map.showMap(mapAccessor);
      else
         showPending = true;

      map.addMapListener(new GeoMapListener() {
         @Override
         public void onZoom(int zoomLevel)
         {
            ObjectGeoLocationView.this.zoomLevel = zoomLevel;
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

      actionHideOtherObjects = new Action(i18n.tr("&Hide other objects"), ResourceManager.getImageDescriptor("icons/hide_other_objects.png")) {
         @Override
         public void run()
         {
            hideOtherObjects = isChecked();
            map.setRootObjectId(hideOtherObjects ? getObjectId() : 0);
            map.setSingleObjectMode(hideOtherObjects);
            map.reloadMap();
         }
      };
      actionHideOtherObjects.setChecked(hideOtherObjects);

      actionUpdateObjectLocation = new Action(i18n.tr("&Set new location here"), ResourceManager.getImageDescriptor("icons/worldmap/geo-pin.png")) {
         @Override
         public void run()
         {
            updateObjectLocation();
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
    * Create pop-up menu for variable list
    */
   private void createPopupMenu()
   {
      MenuManager menuMgr = new ObjectContextMenuManager(this, map, null) {
         @Override
         protected void fillContextMenu()
         {
            if (map.getSelection().isEmpty())
            {
               ObjectGeoLocationView.this.fillContextMenu(this);
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
      manager.add(actionHideOtherObjects);
      manager.add(new Separator());
      manager.add(actionUpdateObjectLocation);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(new Separator());
      manager.add(actionHideOtherObjects);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(new Separator());
      manager.add(actionHideOtherObjects);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      settings.set(getBaseId() + ".hideOtherObjects", hideOtherObjects);
      settings.set(getBaseId() + ".zoom", mapAccessor.getZoom());
      super.dispose();
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

   /**
    * Set object location to the point currently under mouse cursor
    */
   private void updateObjectLocation()
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(getObjectId());
      md.setGeolocation(map.getLocationAtPoint(map.getCurrentPoint()));
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
