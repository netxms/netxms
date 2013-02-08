package org.netxms.agent.android.helpers;

import java.util.regex.Pattern;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.os.Build;
import android.provider.Settings.Secure;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.util.Patterns;

public class DeviceInfoHelper
{
	private static final String TAG = "DeviceInfoHelper";

	/**
	 * Get device serial number (starting from HoneyComb)
	 * 
	 * @return Device serial number if available 
	 */
	@SuppressLint("NewApi")
	static public String getSerial()
	{
		return Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB ? Build.SERIAL : "";
	}

	/**
	 * Get device manufacturer
	 * 
	 * @return Device manufacturer 
	 */
	static public String getManufacturer()
	{
		return Build.MANUFACTURER;
	}

	/**
	 * Get device model
	 * 
	 * @return Device model
	 */
	static public String getModel()
	{
		return Build.MODEL;
	}

	/**
	 * Get device OS relese
	 * 
	 * @return OS release number
	 */
	static public String getRelease()
	{
		return Build.VERSION.RELEASE;
	}

	/**
	 * Get device user
	 * 
	 * @param context	context on which to operate
	 * @return main Google account user
	 */
	static public String getUser(Context context)
	{
		if (context != null)
		{
			try
			{
				Pattern emailPattern = Patterns.EMAIL_ADDRESS; // API level 8+
				Account[] accounts = AccountManager.get(context).getAccounts();
				for (Account account : accounts)
					if (emailPattern.matcher(account.name).matches())
						return account.name;
			}
			catch (Exception e)
			{
				Log.e(TAG, "Exception in getUser()", e);
			}
		}
		return "";
	}

	/**
	 * Get device id
	 * 
	 * @param context	context on which to operate
	 * @return IMEI number or Android ID for phoneless devices 
	 */
	static public String getDeviceId(Context context)
	{
		String id = "";
		if (context != null)
		{
			TelephonyManager tman = (TelephonyManager)context.getSystemService(Context.TELEPHONY_SERVICE);
			id = (tman != null) ? tman.getDeviceId() : null;
			if (id == null)
			{
				id = Secure.getString(context.getContentResolver(), Secure.ANDROID_ID);
			}
		}
		return id;
	}

	/**
	 * Get OS nickname
	 * 
	 * @return OS nick name 
	 */
	static public String getOSName()
	{
		switch (Build.VERSION.SDK_INT)
		{
			case Build.VERSION_CODES.BASE:
				return "ANDROID (BASE)";
			case Build.VERSION_CODES.BASE_1_1:
				return "ANDROID (BASE_1_1)";
			case Build.VERSION_CODES.CUR_DEVELOPMENT:
				return "ANDROID (CUR_DEVELOPMENT)";
			case Build.VERSION_CODES.DONUT:
				return "ANDROID (DONUT)";
			case Build.VERSION_CODES.ECLAIR:
				return "ANDROID (ECLAIR)";
			case Build.VERSION_CODES.ECLAIR_0_1:
				return "ANDROID (ECLAIR_0_1)";
			case Build.VERSION_CODES.ECLAIR_MR1:
				return "ANDROID (ECLAIR_MR1)";
			case Build.VERSION_CODES.FROYO:
				return "ANDROID (FROYO)";
			case Build.VERSION_CODES.GINGERBREAD:
				return "ANDROID (GINGERBREAD)";
			case Build.VERSION_CODES.GINGERBREAD_MR1:
				return "ANDROID (GINGERBREAD_MR1)";
			case Build.VERSION_CODES.HONEYCOMB:
				return "ANDROID (HONEYCOMB)";
			case Build.VERSION_CODES.HONEYCOMB_MR1:
				return "ANDROID (HONEYCOMB_MR1)";
			case Build.VERSION_CODES.HONEYCOMB_MR2:
				return "ANDROID (HONEYCOMB_MR2)";
			case Build.VERSION_CODES.ICE_CREAM_SANDWICH:
				return "ANDROID (ICE_CREAM_SANDWICH)";
			case Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1:
				return "ANDROID (ICE_CREAM_SANDWICH_MR1)";
			case Build.VERSION_CODES.JELLY_BEAN:
				return "ANDROID (JELLY_BEAN)";
			case Build.VERSION_CODES.JELLY_BEAN_MR1:
				return "ANDROID (JELLY_BEAN_MR1)";
		}
		return "ANDROID (UNKNOWN)";
	}

	/**
	 * Get battery level as percentage
	 * 
	 * @param context	context on which to operate
	 * @return battery level (percentage), -1 if value is not available
	 */
	static public int getBatteryLevel(Context context)
	{
		double level = -1;
		if (context != null)
		{
			Intent battIntent = context.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
			if (battIntent != null)
			{
				int rawlevel = battIntent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
				double scale = battIntent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
//				int temp = battIntent.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, -1);
//	            int voltage = battIntent.getIntExtra(BatteryManager.EXTRA_VOLTAGE, -1);
				if (rawlevel >= 0 && scale > 0)
					level = (rawlevel * 100) / scale + 0.5; // + 0.5 for round on cast
			}
		}
		return (int)level;
	}
}
