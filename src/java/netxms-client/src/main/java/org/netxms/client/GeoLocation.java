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
package org.netxms.client;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * @author Victor Kirhenshtein
 *
 */
public class GeoLocation
{
	public static final int UNSET = 0;
	public static final int MANUAL = 1;
	public static final int GPS = 2;
	
	private int type;
	private double latitude;
	private double longitude;
	
	/**
	 * Create geolocation object from NXCP message
	 * @param msg NXCP message
	 */
	public GeoLocation(final NXCPMessage msg)
	{
		type = msg.getVariableAsInteger(NXCPCodes.VID_GEOLOCATION_TYPE);
		latitude = msg.getVariableAsReal(NXCPCodes.VID_LATITUDE);
		longitude = msg.getVariableAsReal(NXCPCodes.VID_LONGITUDE);
	}
	
	/**
	 * Create geolocation object of type UNSET
	 */
	public GeoLocation()
	{
		type = UNSET;
		latitude = 0.0;
		longitude = 0.0;
	}
	
	/**
	 * Create geolocation object of type MANUAL
	 * 
	 * @param lat Latitude
	 * @param lon Longitude
	 */
	public GeoLocation(double lat, double lon)
	{
		type = MANUAL;
		latitude = lat;
		longitude = lon;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the latitude
	 */
	public double getLatitude()
	{
		return latitude;
	}

	/**
	 * @return the longitude
	 */
	public double getLongitude()
	{
		return longitude;
	}
}
