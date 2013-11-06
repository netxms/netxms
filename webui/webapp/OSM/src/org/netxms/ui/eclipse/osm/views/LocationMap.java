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

import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.osm.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Geolocation view
 */
public class LocationMap extends AbstractGeolocationView
{
	public static final String ID = "org.netxms.ui.eclipse.osm.views.LocationMap"; //$NON-NLS-1$
	
	private AbstractObject object;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		try
		{
			long id = Long.parseLong(site.getSecondaryId());
			object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(id);
			setPartName(Messages.LocationMap_PartNamePrefix + object.getObjectName());
		}
		catch(Exception e)
		{
			throw new PartInitException(Messages.LocationMap_InitError1, e);
		}
		if (object == null)
			throw new PartInitException(Messages.LocationMap_InitError2);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#getInitialCenterPoint()
	 */
	@Override
	protected GeoLocation getInitialCenterPoint()
	{
		return object.getGeolocation();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.osm.views.AbstractGeolocationView#getInitialZoomLevel()
	 */
	@Override
	protected int getInitialZoomLevel()
	{
		// TODO Auto-generated method stub
		return 15;
	}
}
