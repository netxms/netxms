/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.handlers.IHandlerService;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.osm.Activator;
import org.netxms.ui.eclipse.osm.Messages;
import org.netxms.ui.eclipse.osm.tools.MapAccessor;
import org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer;
import org.netxms.ui.eclipse.osm.widgets.ObjectGeoLocationViewer;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.FilterText;

/**
 * World map view
 */
public class WorldMap extends AbstractGeolocationView
{
	public static final String ID = "org.netxms.ui.eclipse.osm.views.WorldMap"; //$NON-NLS-1$
	
	private GeoLocation initialLocation = new GeoLocation(0.0, 0.0);
	private int initialZoom;
	private Action actionPlaceObject;
   private Action actionShowFilter;
   private boolean filterEnabled;
   private FilterText filterControl;

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      
      initialZoom = settings.get(ID + "zoom") != null ? settings.getInt(ID + "zoom") : 2;
      if (settings.get(ID + "latitude") != null && settings.get(ID + "longitude") != null)
         initialLocation = new GeoLocation(settings.getDouble(ID + "latitude"), settings.getDouble(ID + "longitude"));
      filterEnabled = settings.get(ID + "filterEnabled") != null ? settings.getBoolean(ID + "filterEnabled") : true;
      
      super.createPartControl(parent);
      
      parent.setLayout(new FormLayout());
      
      // Create filter area
      filterControl = new FilterText(parent, SWT.NONE);
      filterControl.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
      filterControl.setCloseAction(new Action() {
         @Override
         public void run()
         {
            enableFilter(false);
         }
      });
      
      // Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterControl);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      map.setLayoutData(fd);

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterControl.setLayoutData(fd);
      
      // Set initial focus to filter input line
      if (filterEnabled)
         filterControl.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#createMapViewer(org.eclipse.swt.widgets.Composite, int)
    */
   @Override
   protected AbstractGeoMapViewer createMapViewer(Composite parent, int style)
   {
      return new ObjectGeoLocationViewer(parent, style);
   }

	/* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      MapAccessor m = getMapAccessor();

      settings.put(ID + "zoom", m.getZoom());
      settings.put(ID + "latitude", m.getLatitude());
      settings.put(ID + "longitude", m.getLongitude());
      settings.put(ID + "filterEnabled", filterEnabled);
      super.dispose();
   }

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#getInitialCenterPoint()
	 */
	@Override
	protected GeoLocation getInitialCenterPoint()
	{
		return initialLocation;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#getInitialZoomLevel()
	 */
	@Override
	protected int getInitialZoomLevel()
	{
		return initialZoom;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#createActions()
	 */
	@Override
	protected void createActions()
	{
		super.createActions();
		
		actionPlaceObject = new Action(Messages.get().WorldMap_PlaceObject) {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				placeObject();
			}
		};
		
      actionShowFilter = new Action("Show filter", Action.AS_CHECK_BOX) {
         /* (non-Javadoc)
          * @see org.eclipse.jface.action.Action#run()
          */
         @Override
         public void run()
         {
            enableFilter(actionShowFilter.isChecked());
         }
      };
      
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(filterEnabled);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.worldmap.commands.show_filter"); //$NON-NLS-1$
      final ActionHandler showFilterHandler = new ActionHandler(actionShowFilter);
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), showFilterHandler);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#fillContextMenu(org.eclipse.jface.action.IMenuManager)
	 */
	@Override
	protected void fillContextMenu(IMenuManager manager)
	{
		super.fillContextMenu(manager);
		
		if (getSelection().isEmpty())
		{
   		manager.add(new Separator());
   		manager.add(actionPlaceObject);
         manager.add(actionShowFilter);
		}
	}
	
	/* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#fillLocalPullDown(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalPullDown(IMenuManager manager)
   {
      super.fillLocalPullDown(manager);
      manager.add(actionShowFilter);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      super.fillLocalToolBar(manager);
      manager.add(actionShowFilter);
   }

   /**
	 * Place object on map at mouse position
	 */
	private void placeObject()
	{
		final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getSite().getShell(), null, ObjectSelectionDialog.createNodeSelectionFilter(true));
		if (dlg.open() == Window.OK)
		{
			final NXCObjectModificationData md = new NXCObjectModificationData(dlg.getSelectedObjects().get(0).getObjectId());
			md.setGeolocation(map.getLocationAtPoint(map.getCurrentPoint()));
			final NXCSession session = ConsoleSharedData.getSession();
			new ConsoleJob(Messages.get().WorldMap_JobTitle, this, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.modifyObject(md);
				}

				@Override
				protected String getErrorMessage()
				{
					return Messages.get().WorldMap_JobError;
				}
			}.start();
		}
	}
	
	/**
	 * On filter modify
	 */
	private void onFilterModify()
	{
	   ((ObjectGeoLocationViewer)map).setFilterString(filterControl.getText().trim().toLowerCase());
	   map.reloadMap();
	}
	
   /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   public void enableFilter(boolean enable)
   {
      actionShowFilter.setChecked(enable);
      filterEnabled = enable;
      filterControl.setVisible(filterEnabled);
      FormData fd = (FormData)map.getLayoutData();
      fd.top = enable ? new FormAttachment(filterControl) : new FormAttachment(0, 0);
      map.getParent().layout();
      if (enable)
         filterControl.setFocus();
      else
         filterControl.setText(""); //$NON-NLS-1$
   }
}

