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
package org.netxms.ui.eclipse.osm.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.osm.Activator;
import org.netxms.ui.eclipse.osm.Messages;
import org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer;
import org.netxms.ui.eclipse.osm.widgets.ObjectGeoLocationViewer;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Geolocation view
 */
public class LocationMap extends AbstractGeolocationView
{
	public static final String ID = "org.netxms.ui.eclipse.osm.views.LocationMap"; //$NON-NLS-1$
	
	private AbstractObject object;
   private Action actionHideOtherObjects;
   private Action actionShowObjectNames;
   private Action actionUpdateObjectLocation;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		try
		{
			long id = Long.parseLong(site.getSecondaryId());
         object = ConsoleSharedData.getSession().findObjectById(id);
         setPartName(Messages.get().LocationMap_PartNamePrefix + object.getNameWithAlias());
		}
		catch(Exception e)
		{
			throw new PartInitException(Messages.get().LocationMap_InitError1, e);
		}

		if (object == null)
			throw new PartInitException(Messages.get().LocationMap_InitError2);
	}

   /**
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#createMapViewer(org.eclipse.swt.widgets.Composite, int)
    */
   @Override
   protected AbstractGeoMapViewer createMapViewer(Composite parent, int style)
   {
      ObjectGeoLocationViewer mapViewer = new ObjectGeoLocationViewer(parent, style);
      mapViewer.setShowObjectNames(!Activator.getDefault().getDialogSettings().getBoolean("LocationMap.HideObjectNames"));
      if (Activator.getDefault().getDialogSettings().getBoolean("LocationMap.HideOtherObjects"))
      {
         mapViewer.setRootObjectId(object.getObjectId());
         mapViewer.setSingleObjectMode(true);
      }
      return mapViewer;
   }

   /**
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#getInitialCenterPoint()
    */
	@Override
	protected GeoLocation getInitialCenterPoint()
	{
		return object.getGeolocation();
	}

   /**
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#getInitialZoomLevel()
    */
	@Override
	protected int getInitialZoomLevel()
	{
		return 15;
	}

   /**
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#createActions()
    */
   @Override
   protected void createActions()
   {
      super.createActions();

      actionHideOtherObjects = new Action("&Hide other objects", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            Activator.getDefault().getDialogSettings().put("LocationMap.HideOtherObjects", isChecked());
            ((ObjectGeoLocationViewer)map).setRootObjectId(isChecked() ? object.getObjectId() : 0);
            ((ObjectGeoLocationViewer)map).setSingleObjectMode(isChecked());
            map.reloadMap();
         }
      };
      actionHideOtherObjects.setImageDescriptor(Activator.getImageDescriptor("icons/hide_other_objects.png"));
      actionHideOtherObjects.setChecked(Activator.getDefault().getDialogSettings().getBoolean("LocationMap.HideOtherObjects"));

      actionShowObjectNames = new Action("Show object &names", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            Activator.getDefault().getDialogSettings().put("LocationMap.HideObjectNames", !isChecked());
            ((ObjectGeoLocationViewer)map).setShowObjectNames(isChecked());
            map.reloadMap();
         }
      };
      actionShowObjectNames.setImageDescriptor(Activator.getImageDescriptor("icons/show_names.png"));
      actionShowObjectNames.setChecked(!Activator.getDefault().getDialogSettings().getBoolean("LocationMap.HideObjectNames"));

      actionUpdateObjectLocation = new Action("&Set new location here") {
         @Override
         public void run()
         {
            updateObjectLocation();
         }
      };
   }

   /**
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#fillLocalPullDown(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalPullDown(IMenuManager manager)
   {
      super.fillLocalPullDown(manager);
      manager.add(new Separator());
      manager.add(actionHideOtherObjects);
      manager.add(actionShowObjectNames);
   }

   /**
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      super.fillLocalToolBar(manager);
      manager.add(new Separator());
      manager.add(actionHideOtherObjects);
      manager.add(actionShowObjectNames);
   }

   /**
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#fillContextMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillContextMenu(IMenuManager manager)
   {
      super.fillContextMenu(manager);
      manager.add(new Separator());
      manager.add(actionHideOtherObjects);
      manager.add(actionShowObjectNames);
      manager.add(new Separator());
      manager.add(actionUpdateObjectLocation);
   }

   /**
    * Set object location to the point currently under mouse cursor
    */
   private void updateObjectLocation()
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setGeolocation(map.getLocationAtPoint(map.getCurrentPoint()));
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Update object's geolocation", this, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update object's geolocation";
         }
      }.start();
   }
}
