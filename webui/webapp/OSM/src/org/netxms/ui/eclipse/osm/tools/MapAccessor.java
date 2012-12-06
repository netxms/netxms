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
package org.netxms.ui.eclipse.osm.tools;

import org.netxms.base.GeoLocation;

/**
 * This class can be used to build map access URL
 */
public class MapAccessor
{
	public static final int MAX_MAP_WIDTH = 4096;
	public static final int MAX_MAP_HEIGHT = 4096;
	
	private double latitude = 0.0;
	private double longitude = 0.0;
	private int mapWidth = 640;
	private int mapHeight = 640;
	private int zoom = 10;
	private double spanLat = 0;
	private double spanLon = 0;
	private boolean sensor = false;
	
	/**
	 * @param lattitude
	 * @param longitude
	 */
	public MapAccessor(double lattitude, double longitude)
	{
		this.latitude = lattitude;
		this.longitude = longitude;
	}

	/**
	 * @param lattitude
	 * @param longitude
	 * @param zoom
	 */
	public MapAccessor(double lattitude, double longitude, int zoom)
	{
		this.latitude = lattitude;
		this.longitude = longitude;
		this.zoom = zoom;
	}

	/**
	 * @param lattitude
	 * @param longitude
	 * @param mapWidth
	 * @param mapHeight
	 * @param zoom
	 */
	public MapAccessor(double lattitude, double longitude, int zoom, int mapWidth, int mapHeight)
	{
		this.latitude = lattitude;
		this.longitude = longitude;
		this.mapWidth = mapWidth;
		this.mapHeight = mapHeight;
		this.zoom = zoom;
	}

	/**
	 * Copy constructor
	 * 
	 * @param newAccessor
	 */
	public MapAccessor(MapAccessor src)
	{
		latitude = src.latitude;
		longitude = src.longitude;
		mapWidth = src.mapWidth;
		mapHeight = src.mapHeight;
		zoom = src.zoom;
		spanLat = src.spanLat;
		spanLon = src.spanLon;
		sensor = src.sensor;
	}

	/**
	 * Create map accessor from geolocation object
	 * 
	 * @param geolocation
	 */
	public MapAccessor(GeoLocation geolocation)
	{
		latitude = geolocation.getLatitude();
		longitude = geolocation.getLongitude();
		sensor = (geolocation.getType() == GeoLocation.GPS);
	}

	/**
	 * Generate URL to retrieve specified map
	 * @return
	 */
	public String generateUrl()
	{
		//StringBuilder sb = new StringBuilder("http://maps.google.com/staticmap?format=png32&center=");
		StringBuilder sb = new StringBuilder("http://staticmap.openstreetmap.de/staticmap.php?maptype=mapnik&format=png32&center="); //$NON-NLS-1$
		
		// Coordinates of map's center
		sb.append(latitude);
		sb.append(',');
		sb.append(longitude);
		
		// Zoom or span
		if ((spanLat != 0) && (spanLon != 0))
		{
			sb.append("&span="); //$NON-NLS-1$
			sb.append(spanLat);
			sb.append(',');
			sb.append(spanLon);
		}
		else
		{
			sb.append("&zoom="); //$NON-NLS-1$
			sb.append(zoom);
		}
		
		// Map size
		sb.append("&size="); //$NON-NLS-1$
		sb.append(mapWidth);
		sb.append('x');
		sb.append(mapHeight);
		
		return sb.toString();
	}
	
	/**
	 * Check accessor's data for validity
	 * 
	 * @return true if accesor's data is valid
	 */
	public boolean isValid()
	{
		if ((mapWidth < 1) || (mapHeight < 1))
			return false;
		
		if ((zoom < 0) || (zoom > 18))
			return false;
		
		return true;
	}

	/**
	 * @return the latitude
	 */
	public double getLatitude()
	{
		return latitude;
	}

	/**
	 * @param latitude the latitude to set
	 */
	public void setLatitude(double latitude)
	{
		this.latitude = latitude;
	}

	/**
	 * @return the longitude
	 */
	public double getLongitude()
	{
		return longitude;
	}

	/**
	 * @param longitude the longitude to set
	 */
	public void setLongitude(double longitude)
	{
		this.longitude = longitude;
	}

	/**
	 * @return the mapWidth
	 */
	public int getMapWidth()
	{
		return mapWidth;
	}

	/**
	 * @param mapWidth the mapWidth to set
	 */
	public void setMapWidth(int mapWidth)
	{
		this.mapWidth = Math.min(mapWidth, MAX_MAP_WIDTH);
	}

	/**
	 * @return the mapHeight
	 */
	public int getMapHeight()
	{
		return mapHeight;
	}

	/**
	 * @param mapHeight the mapHeight to set
	 */
	public void setMapHeight(int mapHeight)
	{
		this.mapHeight = Math.min(mapHeight, MAX_MAP_HEIGHT);
	}

	/**
	 * @return the zoom
	 */
	public int getZoom()
	{
		return zoom;
	}

	/**
	 * @param zoom the zoom to set
	 */
	public void setZoom(int zoom)
	{
		this.zoom = zoom;
	}

	/**
	 * @return the sensor
	 */
	public boolean isSensor()
	{
		return sensor;
	}

	/**
	 * @param sensor the sensor to set
	 */
	public void setSensor(boolean sensor)
	{
		this.sensor = sensor;
	}

	/**
	 * Set span.
	 * 
	 * @param lat
	 * @param lon
	 */
	public void setSpan(double lat, double lon)
	{
		spanLat = lat;
		spanLon = lon;
	}

	/**
	 * @return the spanLat
	 */
	public double getSpanLatitude()
	{
		return spanLat;
	}

	/**
	 * @return the spanLon
	 */
	public double getSpanLongitude()
	{
		return spanLon;
	}

	/**
	 * @return map's center point
	 */
	public GeoLocation getCenterPoint()
	{
		return new GeoLocation(latitude, longitude);
	}
}
