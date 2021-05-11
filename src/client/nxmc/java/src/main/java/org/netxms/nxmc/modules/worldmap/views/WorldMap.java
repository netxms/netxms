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
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.worldmap.tools.MapAccessor;
import org.netxms.nxmc.modules.worldmap.widgets.AbstractGeoMapViewer;
import org.netxms.nxmc.modules.worldmap.widgets.ObjectGeoLocationViewer;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * World map view
 */
public class WorldMap extends AbstractGeolocationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(WorldMap.class);

   public static final String ID = "WorldMap";
	
	private Action actionPlaceObject;
   private Action actionShowFilter;
   private boolean filterEnabled;

   public WorldMap()
   {
      super(i18n.tr("World Map"), ResourceManager.getImageDescriptor("icons/geomap.png"), ID, true);
   }

   /**
    * @see org.netxms.nxmc.modules.worldmap.views.AbstractGeolocationView#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      super.createContent(parent);
   }

   /**
    * @see org.netxms.nxmc.modules.worldmap.views.AbstractGeolocationView#createMapViewer(org.eclipse.swt.widgets.Composite, int)
    */
   @Override
   protected AbstractGeoMapViewer createMapViewer(Composite parent, int style)
   {
      return new ObjectGeoLocationViewer(parent, style, this);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      MapAccessor m = getMapAccessor();

      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(ID + ".zoom", m.getZoom());
      settings.set(ID + ".latitude", m.getLatitude());
      settings.set(ID + ".longitude", m.getLongitude());
      settings.set(ID + ".filterEnabled", filterEnabled);

      super.dispose();
   }

   /**
    * @see org.netxms.nxmc.modules.worldmap.views.AbstractGeolocationView#getInitialCenterPoint()
    */
	@Override
	protected GeoLocation getInitialCenterPoint()
	{
      PreferenceStore settings = PreferenceStore.getInstance();
      return new GeoLocation(settings.getAsDouble(ID + ".latitude", 0), settings.getAsDouble(ID + ".longitude", 0));
	}

   /**
    * @see org.netxms.nxmc.modules.worldmap.views.AbstractGeolocationView#getInitialZoomLevel()
    */
	@Override
	protected int getInitialZoomLevel()
	{
      return PreferenceStore.getInstance().getAsInteger(ID + ".zoom", 3);
	}

   /**
    * @see org.netxms.nxmc.modules.worldmap.views.AbstractGeolocationView#createActions()
    */
	@Override
	protected void createActions()
	{
		super.createActions();

      actionPlaceObject = new Action(i18n.tr("&Place object here...")) {
			@Override
			public void run()
			{
				placeObject();
			}
		};

      actionShowFilter = new Action("Show filter", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };

      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(filterEnabled);
	}

   /**
    * @see org.netxms.nxmc.modules.worldmap.views.AbstractGeolocationView#fillContextMenu(org.eclipse.jface.action.IMenuManager)
    */
	@Override
	protected void fillContextMenu(IMenuManager manager)
	{
		super.fillContextMenu(manager);
      manager.add(new Separator());
      manager.add(actionPlaceObject);
      manager.add(actionShowFilter);
	}
	
   /**
    * @param manager
    */
   @Override
   protected void fillLocalToolbar(ToolBarManager manager)
   {
      super.fillLocalToolbar(manager);
      manager.add(new Separator());
      manager.add(actionShowFilter);
   }

   /**
	 * Place object on map at mouse position
	 */
	private void placeObject()
	{
      final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getWindow().getShell(),
            ObjectSelectionDialog.createDataCollectionTargetSelectionFilter());
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
	 * On filter modify
	 */
	protected void onFilterModify()
	{
	   ((ObjectGeoLocationViewer)map).setFilterString(filterText.getText().trim().toLowerCase());
	   map.reloadMap();
	}
}
