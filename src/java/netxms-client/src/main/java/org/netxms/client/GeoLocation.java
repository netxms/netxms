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

import java.text.NumberFormat;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Geolocation encoding
 */
public class GeoLocation
{
	private static final double ROUND_OFF = 0.00000001;

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
	
	/**
	 * Extract degrees from lat/lon value
	 * 
	 * @param pos
	 * @return
	 */
	private static int getIntegerDegree(double pos)
	{
		return (int)(Math.abs(pos) + ROUND_OFF);
	}
	
	/**
	 * Extract minutes from lat/lon value
	 * 
	 * @param pos
	 * @return
	 */
	private static int getIntegerMinutes(double pos)
	{
		double d = Math.abs(pos) + ROUND_OFF;
		return (int)((d - (double)((int)d)) * 60.0);
	}
	
	/**
	 * Extract seconds from lat/lon value
	 * 
	 * @param pos
	 * @return
	 */
	private static double getDecimalSeconds(double pos)
	{
		double d = Math.abs(pos) * 60.0 + ROUND_OFF;
		return (d - (double)((int)d)) * 60.0;
	}

	/**
	 * Convert latitude or longitude to text
	 * 
	 * @param pos position
	 * @param isLat true if given position is latitude
	 * @return
	 */
	private static String posToText(double pos, boolean isLat)
	{
		if ((pos < -180.0) || (pos > 180.0))
			return "<invalid>";
		
		StringBuilder sb = new StringBuilder();

		// Encode hemisphere
		if (isLat)
		{
			sb.append((pos < 0) ? 'S' : 'N');
		}
		else
		{
			sb.append((pos < 0) ? 'W' : 'E');
		}
		sb.append(' ');
		
		NumberFormat nf = NumberFormat.getIntegerInstance();
		nf.setMinimumIntegerDigits(2);
		sb.append(nf.format(getIntegerDegree(pos)));
		sb.append("° ");
		sb.append(nf.format(getIntegerMinutes(pos)));
		sb.append("' ");

		nf = NumberFormat.getNumberInstance();
		nf.setMinimumIntegerDigits(2);
		nf.setMinimumFractionDigits(3);
		nf.setMaximumFractionDigits(3);
		sb.append(nf.format(getDecimalSeconds(pos)));
		sb.append('"');
		
		return sb.toString();
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		return posToText(latitude, true) + " " + posToText(longitude, false);
	}
}
