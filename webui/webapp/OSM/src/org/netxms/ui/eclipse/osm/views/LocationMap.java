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

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.base.GeoLocation;
import org.netxms.client.objects.AbstractObject;
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
   }

   /**
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#fillLocalPullDown(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionHideOtherObjects);
      manager.add(new Separator());
      super.fillLocalPullDown(manager);
   }

   /**
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionHideOtherObjects);
      manager.add(new Separator());
      super.fillLocalToolBar(manager);
   }
}
