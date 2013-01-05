package org.netxms.agent.android.service.helpers;

import android.location.Location;
import android.location.LocationListener;
import android.os.Bundle;

/**
 * Location manager helper to receive position updates messages
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class LocationManagerHelper implements LocationListener
{

	private static double latitude;
	private static double longitude;

	@Override
	public void onLocationChanged(Location loc)
	{
		latitude = loc.getLatitude();
		longitude = loc.getLongitude();
	}

	@Override
	public void onProviderDisabled(String provider)
	{
	}

	@Override
	public void onProviderEnabled(String provider)
	{
	}

	@Override
	public void onStatusChanged(String provider, int status, Bundle extras)
	{
		// TODO Auto-generated method stub

	}

	public static double getLatitude()
	{
		return latitude;
	}

	public static double getLongitude()
	{
		return longitude;
	}

}