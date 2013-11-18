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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.osm.Activator;
import org.netxms.ui.eclipse.osm.Messages;
import org.netxms.ui.eclipse.osm.tools.MapAccessor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * World map view
 */
public class WorldMap extends AbstractGeolocationView
{
	public static final String ID = "org.netxms.ui.eclipse.osm.views.WorldMap"; //$NON-NLS-1$
	
	private GeoLocation initialLocation = new GeoLocation(0.0, 0.0);
	private int initialZoom = 2;
	private Action actionPlaceObject;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
	 */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		if (memento != null)
		{
			if (memento.getInteger("zoom") != null) //$NON-NLS-1$
				initialZoom = memento.getInteger("zoom"); //$NON-NLS-1$
			
			Float lat = memento.getFloat("latitude"); //$NON-NLS-1$
			Float lon = memento.getFloat("longitude"); //$NON-NLS-1$
			if ((lat != null) && (lon != null))
				initialLocation = new GeoLocation(lat, lon);
		}		
		super.init(site, memento);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#saveState(org.eclipse.ui.IMemento)
	 */
	@Override
	public void saveState(IMemento memento)
	{
		super.saveState(memento);
		MapAccessor m = getMapAccessor();
		memento.putFloat("latitude", (float)m.getLatitude()); //$NON-NLS-1$
		memento.putFloat("longitude", (float)m.getLongitude()); //$NON-NLS-1$
		memento.putInteger("zoom", m.getZoom()); //$NON-NLS-1$
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
			@Override
			public void run()
			{
				placeObject();
			}
		};
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#fillContextMenu(org.eclipse.jface.action.IMenuManager)
	 */
	@Override
	protected void fillContextMenu(IMenuManager manager)
	{
		super.fillContextMenu(manager);
		manager.add(new Separator());
		manager.add(actionPlaceObject);
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
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
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
}

