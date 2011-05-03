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
package org.netxms.ui.eclipse.osm;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.swt.graphics.Point;
import org.eclipse.ui.IStartup;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.GeoLocation;
import org.netxms.client.NXCNotification;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.osm.tools.Area;
import org.netxms.ui.eclipse.osm.tools.QuadTree;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Cache for objects' geolocation information
 */
public class GeoLocationCache implements IStartup, SessionListener
{
	private static final int TILE_SIZE = 256;
	
	private static GeoLocationCache instance = null;
	
	private Map<Long, GenericObject> objects = new HashMap<Long, GenericObject>();
	private QuadTree<Long> locationTree = new QuadTree<Long>();
	
	/**
	 * Default constructor
	 */
	public GeoLocationCache()
	{
		instance = this;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IStartup#earlyStartup()
	 */
	@Override
	public void earlyStartup()
	{
		while(ConsoleSharedData.getSession() == null)
		{
			try
			{
				Thread.sleep(1000);
			}
			catch(InterruptedException e)
			{
			}
		}
		ConsoleSharedData.getSession().addListener(this);
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(SessionNotification n)
	{
		if (n.getCode() == NXCNotification.OBJECT_CHANGED)
			onObjectChange((GenericObject)n.getObject());
	}
	
	/**
	 * Handle object change
	 * 
	 * @param object
	 */
	private void onObjectChange(GenericObject object)
	{
		if ((object.getObjectClass() != GenericObject.OBJECT_NODE) ||
		    (object.getObjectClass() != GenericObject.OBJECT_CLUSTER) ||
		    (object.getObjectClass() != GenericObject.OBJECT_CONTAINER))
			return;
		
		GeoLocation gl = object.getGeolocation();
		if (gl.getType() == GeoLocation.UNSET)
		{
			objects.remove(object.getObjectId());
		}
		else
		{
			if (!objects.containsKey(object.getObjectId()))
			{
				locationTree.insert(gl.getLatitude(), gl.getLongitude(), object.getObjectId());
			}
			else
			{
				if (!gl.equals(objects.get(object.getObjectId()).getGeolocation()))
				{
					
				}
			}
			objects.put(object.getObjectId(), object);
		}
	}
	
	/**
	 * Convert location to abstract display coordinates (coordinates on virtual map covering entire world)
	 * 
	 * @param location geolocation
	 * @param zoom zoom level
	 * @return abstract display coordinates
	 */
	private static Point coordinateToDisplay(GeoLocation location, int zoom)
	{
		final double numberOfTiles = Math.pow(2, zoom);
		
		// LonToX
		double x = (location.getLongitude() + 180) * (numberOfTiles * TILE_SIZE) / 360.0;
	    
		// LatToY
		double y = 1 - (Math.log(Math.tan(Math.PI / 4 + Math.toRadians(location.getLatitude()) / 2)) / Math.PI);
		y = y / 2 * (numberOfTiles * TILE_SIZE);
		return new Point((int)x, (int)y);
	}
	
	/**
	 * Convert abstract display coordinates (coordinates on virtual map covering entire world) to location
	 * 
	 * @param point display coordinates
	 * @param zoom zoom level
	 * @return location for given point
	 */
	private static GeoLocation displayToCoordinates(Point point, int zoom)
	{
		double longitude = (point.x *(360 / (Math.pow(2, zoom) * 256))) - 180;
		
		double latitude = (1 - (point.y * (2 / (Math.pow(2, zoom) * 256)))) * Math.PI;
		latitude = Math.toDegrees(Math.atan(Math.sinh(latitude)));
		
		return new GeoLocation(latitude, longitude);
	}
	
	/**
	 * Calculate map coverage.
	 * 
	 * @param mapSize map size in pixels
	 * @param centerPoint coordinates of map's center
	 * @param zoom zoom level
	 * @return area covered by map
	 */
	public static Area calculateCoverage(Point mapSize, GeoLocation centerPoint, int zoom)
	{
		Point cp = coordinateToDisplay(centerPoint, zoom);
		GeoLocation topLeft = displayToCoordinates(new Point(cp.x - mapSize.x / 2, cp.y - mapSize.y / 2), zoom);
		GeoLocation bottomRight = displayToCoordinates(new Point(cp.x + mapSize.x / 2, cp.y + mapSize.y / 2), zoom);
		return new Area(topLeft.getLatitude(), topLeft.getLongitude(), bottomRight.getLatitude(), bottomRight.getLongitude());
	}
}
