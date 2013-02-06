/**
 * 
 */
package org.netxms.agent.android.service;

import java.io.IOException;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Calendar;
import java.util.Enumeration;
import java.util.regex.Pattern;

import org.netxms.agent.android.R;
import org.netxms.agent.android.helpers.SafeParser;
import org.netxms.agent.android.main.activities.HomeScreen;
import org.netxms.agent.android.receivers.AlarmIntentReceiver;
import org.netxms.agent.android.service.helpers.AndroidLoggingFacility;
import org.netxms.base.GeoLocation;
import org.netxms.base.Logger;
import org.netxms.mobile.agent.MobileAgentException;
import org.netxms.mobile.agent.Session;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.annotation.SuppressLint;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.BatteryManager;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.provider.Settings.Secure;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.util.Patterns;
import android.widget.Toast;

/**
 * Background communication service for NetXMS agent.
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class AgentConnectorService extends Service implements LocationListener
{
	public enum ConnectionStatus
	{
		CS_NOCONNECTION, CS_INPROGRESS, CS_ALREADYCONNECTED, CS_CONNECTED, CS_DISCONNECTED, CS_ERROR
	};

	public static final String ACTION_CONNECT = "org.netxms.agent.android.ACTION_CONNECT";
	public static final String ACTION_FORCE_CONNECT = "org.netxms.agent.android.ACTION_FORCE_CONNECT";
	public static final String ACTION_SCHEDULE = "org.netxms.agent.android.ACTION_SCHEDULE";
	public static final String ACTION_CONFIGURE = "org.netxms.agent.android.ACTION_CONFIGURE";

	public static boolean gettingNewLocation = false;

	private static final String TAG = "nxagent/AgentConnectorService";
	private static final int ONE_DAY_MINUTES = 24 * 60;
	private static final int NETXMS_REQUEST_CODE = 123456;

	private final Binder binder = new AgentConnectorBinder();
	private Handler uiThreadHandler;
	private Handler locationHandler = null;
	private ConnectionStatus connectionStatus = ConnectionStatus.CS_DISCONNECTED;
	private BroadcastReceiver receiver = null;
	private SharedPreferences sp;
	private LocationManager locationManager = null;
	private HomeScreen homeScreen = null;
	private boolean sendDeviceSystemInfo = true;
	private boolean agentActive;
	private boolean notifyToast;
	private String connectionServer;
	private int connectionPort;
	private String connectionLogin;
	private String connectionPassword;
	private boolean connectionEncrypt;
	private boolean schedulerDaily;
	private int schedulerInterval;
	private boolean locationForce;
	private int locationInterval;
	private int locationDuration;
	private int locationStrategy;

	/**
	 * Class for clients to access. Because we know this service always runs in
	 * the same process as its clients, we don't need to deal with IPC.
	 */
	public class AgentConnectorBinder extends Binder
	{
		public AgentConnectorService getService()
		{
			return AgentConnectorService.this;
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Service#onCreate()
	 */
	@Override
	public void onCreate()
	{
		super.onCreate();
		sp = PreferenceManager.getDefaultSharedPreferences(this);
		Logger.setLoggingFacility(new AndroidLoggingFacility());

		uiThreadHandler = new Handler(getMainLooper());

		locationManager = (LocationManager)getSystemService(Context.LOCATION_SERVICE);
		configure();

		receiver = new BroadcastReceiver()
		{
			@Override
			public void onReceive(Context context, Intent intent)
			{
				Intent i = new Intent(context, AgentConnectorService.class);
				i.setAction(ACTION_SCHEDULE);
				context.startService(i);
			}
		};
		registerReceiver(receiver, new IntentFilter(Intent.ACTION_TIME_TICK));
		showToast(getString(R.string.notify_started));
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Service#onStartCommand(android.content.Intent, int, int)
	 */
	@Override
	public int onStartCommand(Intent intent, int flags, int startId)
	{
		if ((intent != null) && (intent.getAction() != null))
		{
			Log.i(TAG, "onStartCommand: " + intent.getAction());
			if (intent.getAction().equals(ACTION_CONNECT))
				reconnect(false);
			else if (intent.getAction().equals(ACTION_FORCE_CONNECT))
			{
				sendDeviceSystemInfo = true;
				reconnect(true);
			}
			else if (intent.getAction().equals(ACTION_CONFIGURE))
			{
				sendDeviceSystemInfo = true;
				configure();
				reconnect(true);
			}
		}
		return super.onStartCommand(intent, flags, startId);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Service#onBind(android.content.Intent)
	 */
	@Override
	public IBinder onBind(Intent intent)
	{
		return binder;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Service#onDestroy()
	 */
	@Override
	public void onDestroy()
	{
		super.onDestroy();
	}

	/**
	 * Configure background service
	 */
	void configure()
	{
		agentActive = sp.getBoolean("global.activate", false);
		notifyToast = sp.getBoolean("notification.toast", true);
		connectionServer = sp.getString("connection.server", "");
		connectionPort = SafeParser.parseInt(sp.getString("connection.port", "4747"), 4747);
		connectionLogin = sp.getString("connection.login", "");
		connectionPassword = sp.getString("connection.password", "");
		connectionEncrypt = sp.getBoolean("connection.encrypt", false);
		schedulerDaily = sp.getBoolean("scheduler.daily.enable", false);
		schedulerInterval = Integer.parseInt(sp.getString("scheduler.interval", "15"));
		locationStrategy = SafeParser.parseInt(sp.getString("location.strategy", "0"), 0);
		locationInterval = SafeParser.parseInt(sp.getString("location.interval", "30"), 30) * 60 * 1000;
		locationDuration = SafeParser.parseInt(sp.getString("location.duration", "2"), 2) * 60 * 1000;
		locationForce = sp.getBoolean("location.force", false);
		if (locationManager != null)
		{
			if (locationHandler == null)
				locationHandler = new Handler(getMainLooper());
			if (locationForce)
				locationHandler.post(locationTask);
			else
				locationHandler.removeCallbacks(locationTask);
		}
	}

	/**
	 * Shutdown background service
	 */
	public void shutdown()
	{
		cancelSchedule();
		unregisterReceiver(receiver);
		stopSelf();
	}

	/**
	 * Show status notification
	 * 
	 * @param status	connection status
	 * @param extra	extra text to add at the end of the toast
	 */
	public void statusNotification(ConnectionStatus status, String extra)
	{
		connectionStatus = status;
		String text = "";
		switch (status)
		{
			case CS_CONNECTED:
				text = getString(R.string.notify_connected, extra);
				break;
			case CS_ERROR:
				text = getString(R.string.notify_connection_failed, extra);
				break;
			case CS_NOCONNECTION:
			case CS_INPROGRESS:
			case CS_ALREADYCONNECTED:
			default:
				return;
		}
		if (notifyToast)
			showToast(text);
	}

	/**
	 * Reconnect to server.
	 * 
	 * @param force if set to true forces reconnection bypassing the scheduler
	 */
	public void reconnect(boolean force)
	{
		if (agentActive && (force || isScheduleExpired()) &&
				connectionStatus != ConnectionStatus.CS_INPROGRESS &&
				connectionStatus != ConnectionStatus.CS_CONNECTED)
		{
			new PushDataTask(connectionServer, connectionPort, getDeviceId(), connectionLogin, connectionPassword, connectionEncrypt).execute();
		}
	}

	/**
	 * @param homeScreen
	 */
	public void registerHomeScreen(HomeScreen homeScreen)
	{
		this.homeScreen = homeScreen;
	}

	/**
	 * Refresh homescreen  activity
	 */
	private void refreshHomeScreen()
	{
		if (homeScreen != null)
		{
			homeScreen.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					homeScreen.refreshStatus();
				}
			});
		}
	}

	/**
	 * Check for expired pending connection schedule
	 */
	private boolean isScheduleExpired()
	{
		Calendar cal = Calendar.getInstance(); // get a Calendar object with current time
		return cal.getTimeInMillis() > sp.getLong("scheduler.next_activation", 0);
	}

	/**
	 * Gets stored time settings in minutes
	 */
	private int getMinutes(String time)
	{
		String[] vals = sp.getString(time, "00:00").split(":");
		return Integer.parseInt(vals[0]) * 60 + Integer.parseInt(vals[1]);
	}

	/**
	 * Sets the offset used to compute the next schedule
	 */
	private void setDayOffset(Calendar cal, int minutes)
	{
		cal.set(Calendar.HOUR_OF_DAY, 0);
		cal.set(Calendar.MINUTE, 0);
		cal.set(Calendar.SECOND, 0);
		cal.set(Calendar.MILLISECOND, 0);
		cal.add(Calendar.MINUTE, minutes);
	}

	/**
	 * Schedule a new connection/disconnection
	 */
	private void schedule()
	{
		Calendar cal = Calendar.getInstance(); // get a Calendar object with current time
		if (!schedulerDaily)
			cal.add(Calendar.MINUTE, schedulerInterval);
		else
		{
			int on = getMinutes("scheduler.daily.on");
			int off = getMinutes("scheduler.daily.off");
			if (off < on)
				off += ONE_DAY_MINUTES; // Next day!
			Calendar calOn = (Calendar)cal.clone();
			setDayOffset(calOn, on);
			Calendar calOff = (Calendar)cal.clone();
			setDayOffset(calOff, off);
			cal.add(Calendar.MINUTE, schedulerInterval);
			if (cal.before(calOn))
			{
				cal = (Calendar)calOn.clone();
				Log.i(TAG, "schedule (before): rescheduled for daily interval");
			}
			else if (cal.after(calOff))
			{
				cal = (Calendar)calOn.clone();
				setDayOffset(cal, on + ONE_DAY_MINUTES); // Move to the next activation of the excluded range
				Log.i(TAG, "schedule (after): rescheduled for daily interval");
			}
		}
		setSchedule(cal.getTimeInMillis());
	}

	/**
	 * Set a connection schedule
	 */
	private void setSchedule(long milliseconds)
	{
		Log.i(TAG, "setSchedule to: " + milliseconds);
		Intent intent = new Intent(this, AlarmIntentReceiver.class);
		PendingIntent sender = PendingIntent.getBroadcast(this, NETXMS_REQUEST_CODE, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		((AlarmManager)getSystemService(ALARM_SERVICE)).set(AlarmManager.RTC_WAKEUP, milliseconds, sender);
		Editor e = sp.edit();
		e.putLong("scheduler.next_activation", milliseconds);
		e.commit();
	}

	/**
	 * Cancel a pending connection schedule (if any)
	 */
	private void cancelSchedule()
	{
		Log.i(TAG, "cancelSchedule");
		Intent intent = new Intent(this, AlarmIntentReceiver.class);
		PendingIntent sender = PendingIntent.getBroadcast(this, NETXMS_REQUEST_CODE, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		((AlarmManager)getSystemService(ALARM_SERVICE)).cancel(sender);
		Editor e = sp.edit();
		e.putLong("scheduler.next_activation", 0);
		e.commit();
	}

	/**
	 * Show toast with given text
	 * 
	 * @param text message text
	 */
	public void showToast(final String text)
	{
		uiThreadHandler.post(new Runnable()
		{
			@Override
			public void run()
			{
				Toast.makeText(getApplicationContext(), text, Toast.LENGTH_SHORT).show();
			}
		});
	}

	/**
	 * Get device serial number
	 */
	@SuppressLint("NewApi")
	public String getSerial()
	{
		return Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB ? Build.SERIAL : "";
	}

	/**
	 * Get device manufacturer
	 */
	public String getManufacturer()
	{
		return Build.MANUFACTURER;
	}

	/**
	 * Get device model
	 */
	public String getModel()
	{
		return Build.MODEL;
	}

	/**
	 * Get device OS relese
	 */
	public String getRelease()
	{
		return Build.VERSION.RELEASE;
	}

	/**
	 * Get device main Google account user.
	 */
	public String getUser()
	{
		try
		{
			Pattern emailPattern = Patterns.EMAIL_ADDRESS; // API level 8+
			Account[] accounts = AccountManager.get(getApplicationContext()).getAccounts();
			for (Account account : accounts)
				if (emailPattern.matcher(account.name).matches())
					return account.name;
		}
		catch (Exception e)
		{
			Log.e(TAG, "Exception in getUser()", e);
		}
		return "";
	}

	/**
	 * Get device id: IMEI number or Android ID for phoneless devices
	 */
	public String getDeviceId()
	{
		TelephonyManager tman = (TelephonyManager)getSystemService(Context.TELEPHONY_SERVICE);
		String id = (tman != null) ? tman.getDeviceId() : null;
		if (id == null)
		{
			id = Secure.getString(getContentResolver(), Secure.ANDROID_ID);
		}
		return id;
	}

	/**
	 * Get OS nickname
	 */
	public String getOSName()
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
	 * Get internet address. NB must run on background thread (as per honeycomb specs)
	 */
	private InetAddress getInetAddress()
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
	 * Get last known geo location (depending on strategy used it could be very old)
	 * 
	 * @return last known location or null if not known
	 */
	private GeoLocation getGeoLocation()
	{
		if (locationManager != null)
		{
			Criteria hdCrit = new Criteria();
			if (hdCrit != null)
			{
				hdCrit.setAccuracy(Criteria.ACCURACY_COARSE);
				String mlocProvider = locationManager.getBestProvider(hdCrit, true);
				if (mlocProvider != null)
				{
					Location currLocation = locationManager.getLastKnownLocation(mlocProvider);
					if (currLocation != null)
						return new GeoLocation(currLocation.getLatitude(), currLocation.getLongitude());
				}
			}
		}
		return null;
	}

	/**
	 * Get flags. Currently not used.
	 */
	private int getFlags()
	{
		return 0;
	}

	/**
	 * Get battery level as percentage. -1 if value is not available
	 */
	private int getBatteryLevel()
	{
		double level = -1;
		Context context = getApplicationContext();
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

	/**
	 * Check for internet connectivity 
	 */
	private boolean isInternetOn()
	{
		ConnectivityManager conn = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
		if (conn != null)
		{
			NetworkInfo wifi = conn.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
			if (wifi != null && wifi.getState() == NetworkInfo.State.CONNECTED)
				return true;
			NetworkInfo mobile = conn.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
			if (mobile != null && mobile.getState() == NetworkInfo.State.CONNECTED)
				return true;
		}
		return false;
	}

	/**
	 * Internal task for connecting and pushing data
	 */
	private class PushDataTask extends AsyncTask<Object, Void, Boolean>
	{
		private final String server;
		private final Integer port;
		private final String deviceId;
		private final String login;
		private final String password;
		private final boolean encrypt;
		private String connMsg = "";

		protected PushDataTask(String server, Integer port, String deviceId, String login, String password, boolean encrypt)
		{
			this.server = server;
			this.port = port;
			this.deviceId = deviceId;
			this.login = login;
			this.password = password;
			this.encrypt = encrypt;
		}

		@Override
		protected Boolean doInBackground(Object... params)
		{
			Log.d(TAG, "PushDataTask.doInBackground: reconnecting...");
			statusNotification(ConnectionStatus.CS_INPROGRESS, "");
			if (isInternetOn())
			{
				Session session = new Session(server, port, deviceId, login, password, encrypt);
				try
				{
					session.connect();
					Log.d(TAG, "PushDataTask.doInBackground: connected");
					statusNotification(ConnectionStatus.CS_CONNECTED, getString(R.string.notify_pushing_data));
					if (sendDeviceSystemInfo)
					{
						Log.d(TAG, "PushDataTask.doInBackground: sending DeviceSystemInfo");
						session.reportDeviceSystemInfo(getManufacturer(), getModel(), getOSName(), getRelease(), getSerial(), getUser());
						sendDeviceSystemInfo = false;
					}
					session.reportDeviceStatus(getInetAddress(), getGeoLocation(), getFlags(), getBatteryLevel());
					session.disconnect();
					Log.d(TAG, "PushDataTask.doInBackground: data transfer completed");
					connMsg = getString(R.string.notify_connected, server + ":" + port);
					return true;
				}
				catch (IOException e)
				{
					Log.e(TAG, "IOException while executing PushDataTask.doInBackground on connect", e);
					connMsg = e.getLocalizedMessage();
				}
				catch (MobileAgentException e)
				{
					Log.e(TAG, "MobileAgentException while executing PushDataTask.doInBackground on connect", e);
					connMsg = e.getLocalizedMessage();
				}
			}
			else
			{
				Log.w(TAG, "PushDataTask.doInBackground: no internet connection");
				connMsg = getString(R.string.notify_no_connection);
			}
			return false;
		}

		@Override
		protected void onPostExecute(Boolean result)
		{
			if (result == true)
			{
				Log.d(TAG, "PushDataTask.onPostExecute: disconnecting...");
				statusNotification(ConnectionStatus.CS_DISCONNECTED, "");
			}
			else
			{
				Log.e(TAG, "PushDataTask.onPostExecute: error: " + connMsg);
				statusNotification(ConnectionStatus.CS_ERROR, connMsg);
			}
			Editor e = sp.edit();
			e.putLong("scheduler.last_activation", Calendar.getInstance().getTimeInMillis());
			e.putString("scheduler.last_activation_msg", connMsg);
			e.commit();
			if (agentActive)
				schedule();
			else
				cancelSchedule();
			refreshHomeScreen();
		}
	}

	/**
	 * Internal handler to get new location based on location strategy set
	 */
	private final Runnable locationTask = new Runnable()
	{
		@Override
		public void run()
		{
			if (agentActive && locationForce)
			{
				if (AgentConnectorService.gettingNewLocation)
				{
					Log.d(TAG, "Timeout expired while trying to get a new location...");
					locationManager.removeUpdates(AgentConnectorService.this);
					AgentConnectorService.gettingNewLocation = false;
					locationHandler.postDelayed(locationTask, locationInterval);
				}
				else
				{
					String provider = "";
					switch (locationStrategy)
					{
						case 0:
							if (locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
								locationManager.requestLocationUpdates(provider = LocationManager.NETWORK_PROVIDER, locationInterval, 100, AgentConnectorService.this);
							break;
						case 1:
							if (locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
								locationManager.requestLocationUpdates(provider = LocationManager.GPS_PROVIDER, locationInterval, 100, AgentConnectorService.this);
							break;
						case 2:
							if (locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
								locationManager.requestLocationUpdates(provider = LocationManager.NETWORK_PROVIDER, locationInterval, 100, AgentConnectorService.this);
							else if (locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
								locationManager.requestLocationUpdates(provider = LocationManager.GPS_PROVIDER, locationInterval, 100, AgentConnectorService.this);
							break;
						case 3:
							if (locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
								locationManager.requestLocationUpdates(provider = LocationManager.GPS_PROVIDER, locationInterval, 100, AgentConnectorService.this);
							else if (locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
								locationManager.requestLocationUpdates(provider = LocationManager.NETWORK_PROVIDER, locationInterval, 100, AgentConnectorService.this);
							break;
					}
					if (provider.length() > 0)
					{
						Log.d(TAG, "Trying to get a new location from '" + provider + "' provider...");
						AgentConnectorService.gettingNewLocation = true;
						locationHandler.postDelayed(locationTask, locationDuration);
					}
					else
					{
						Log.w(TAG, "No available location provider!");
						locationHandler.postDelayed(locationTask, locationInterval);
					}
				}
			}
			else
			{
				locationHandler.removeCallbacks(locationTask);
				AgentConnectorService.gettingNewLocation = false;
				locationManager.removeUpdates(AgentConnectorService.this);
			}

		}
	};

	@Override
	public void onLocationChanged(Location location)
	{
		Log.d(TAG, "Got a new location at Lat: " + location.getLatitude() + ", Lon: " + location.getLongitude());
		if (gettingNewLocation)
		{
			locationManager.removeUpdates(AgentConnectorService.this);
			gettingNewLocation = false;
			locationHandler.removeCallbacks(locationTask);
			locationHandler.postDelayed(locationTask, locationInterval);
		}
	}

	@Override
	public void onProviderDisabled(String provider)
	{
		// TODO Auto-generated method stub
	}

	@Override
	public void onProviderEnabled(String provider)
	{
		// TODO Auto-generated method stub
	}

	@Override
	public void onStatusChanged(String provider, int status, Bundle extras)
	{
		// TODO Auto-generated method stub
	}
}
