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

import org.eclipse.ui.IMemento;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.client.GeoLocation;
import org.netxms.ui.eclipse.osm.tools.MapAccessor;

/**
 * World map view
 */
public class WorldMap extends AbstractGeolocationView
{
	public static final String ID = "org.netxms.ui.eclipse.osm.views.WorldMap";
	
	private GeoLocation initialLocation = new GeoLocation(0.0, 0.0);
	private int initialZoom = 2;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite, org.eclipse.ui.IMemento)
	 */
	@Override
	public void init(IViewSite site, IMemento memento) throws PartInitException
	{
		if (memento.getInteger("zoom") != null)
			initialZoom = memento.getInteger("zoom");
		
		Float lat = memento.getFloat("latitude");
		Float lon = memento.getFloat("longitude");
		if ((lat != null) && (lon != null))
			initialLocation = new GeoLocation(lat, lon);
		
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
		memento.putFloat("latitude", (float)m.getLatitude());
		memento.putFloat("longitude", (float)m.getLongitude());
		memento.putInteger("zoom", m.getZoom());
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
}

