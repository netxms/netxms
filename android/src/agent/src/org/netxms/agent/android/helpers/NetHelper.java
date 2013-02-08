package org.netxms.agent.android.helpers;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Enumeration;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

public class NetHelper
{
	private static final String TAG = "NetHelper";

	/**
	 * Get internet address. NB must run on background thread (as per honeycomb specs)
	 * 
	 * @return Internet address, null if no addresses are available
	 */
	static public InetAddress getInetAddress()
	{
		try
		{
			Enumeration<NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces();
			while (interfaces.hasMoreElements())
			{
				NetworkInterface iface = interfaces.nextElement();
				Enumeration<InetAddress> addrList = iface.getInetAddresses();
				while (addrList.hasMoreElements())
				{
					InetAddress addr = addrList.nextElement();
					if (!addr.isAnyLocalAddress() && !addr.isLinkLocalAddress() && !addr.isLoopbackAddress() && !addr.isMulticastAddress())
						return addr;
				}
			}
			return InetAddress.getLocalHost();
		}
		catch (Exception e)
		{
			Log.e(TAG, "Exception in getInetAddress()", e);
		}
		return null;
	}

	/**
	 * Check for internet connectivity 
	 * 
	 * @param context	context on which to operate
	 * @return true if access to internet is available
	 */
	static public boolean isInternetOn(Context context)
	{
		if (context != null)
		{
			ConnectivityManager conn = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
			if (conn != null)
			{
				NetworkInfo wifi = conn.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
				if (wifi != null && wifi.getState() == NetworkInfo.State.CONNECTED)
					return true;
				NetworkInfo mobile = conn.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
				if (mobile != null && mobile.getState() == NetworkInfo.State.CONNECTED)
					return true;
			}
		}
		return false;
	}
}
