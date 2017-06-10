/**
 * 
 */
package org.netxms.agent.android.service;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;

import org.netxms.agent.android.R;
import org.netxms.agent.android.helpers.DeviceInfoHelper;
import org.netxms.agent.android.helpers.NetHelper;
import org.netxms.agent.android.helpers.SafeParser;
import org.netxms.agent.android.helpers.SharedPrefs;
import org.netxms.agent.android.helpers.TimeHelper;
import org.netxms.agent.android.main.activities.HomeScreen;
import org.netxms.agent.android.receivers.AlarmIntentReceiver;
import org.netxms.agent.android.service.helpers.AndroidLoggingFacility;
import org.netxms.base.GeoLocation;
import org.netxms.base.Logger;
import org.netxms.mobile.agent.MobileAgentException;
import org.netxms.mobile.agent.Session;

import android.Manifest;
import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.support.v4.app.NotificationCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
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
    public static final String PERMISSIONS_GRANTED = "PermissionsGranted";
    public static final String SCHEDULER_NEXT_ACTIVATION = "scheduler.next_activation";
    public static final String SCHEDULER_LAST_ACTIVATION = "scheduler.last_activation";
    public static final String SCHEDULER_LAST_ACTIVATION_MSG = "scheduler.last_activation_msg";
    public static final String LOCATION_LAST_STATUS = "location.last_status";

	public static boolean gettingNewLocation = false;

	private static final String TAG = "NxAgent/AgentConnectorS";
	private static final int ONE_DAY_MINUTES = 24 * 60;
	private static final int NETXMS_REQUEST_CODE = 123456;
	private static final int STRATEGY_NET_ONLY = 0;
	private static final int STRATEGY_GPS_ONLY = 1;
	private static final int STRATEGY_NET_AND_GPS = 2;
	private static final int NOTIFY_STATUS = 1;
	private static final int NOTIFY_STATUS_NEVER = 0;
	private static final int NOTIFY_STATUS_ON_CONNECT = 1;
	private static final int NOTIFY_STATUS_ON_DISCONNECT = 2;
	private static final int NOTIFY_STATUS_ALWAYS = 3;

	private final Binder binder = new AgentConnectorBinder();
	private Handler uiThreadHandler = null;
	private Handler locationHandler = null;
	private ConnectionStatus connectionStatus = ConnectionStatus.CS_DISCONNECTED;
	private BroadcastReceiver receiver = null;
	private SharedPreferences sp;
	private LocationManager locationManager = null;
	private NotificationManager notificationManager;
	private HomeScreen homeScreen = null;
	private boolean sendDeviceSystemInfo = true;
	private boolean agentActive;
	private boolean notifyToast;
	private boolean notifyIcon;
	private int notificationType = NOTIFY_STATUS_NEVER;
	private String connectionServer;
	private int connectionPort;
	private String connectionLogin;
	private String connectionPassword;
	private boolean connectionEncrypt;
	private boolean schedulerDaily;
	private int connectionInterval;
	private boolean locationForce;
	private int locationInterval;
	private int locationDuration;
	private int locationStrategy;
	private String locationProvider = "";
	private String allowedProviders = "";

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
		uiThreadHandler = new Handler(getMainLooper());
		showToast(getString(R.string.notify_started));

		sp = PreferenceManager.getDefaultSharedPreferences(this);

        // Check for permissions
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED ||
                ContextCompat.checkSelfPermission(this, Manifest.permission.GET_ACCOUNTS) != PackageManager.PERMISSION_GRANTED)
            SharedPrefs.writeBoolean(sp, PERMISSIONS_GRANTED, false);
        else
            SharedPrefs.writeBoolean(sp, PERMISSIONS_GRANTED, true);

		Logger.setLoggingFacility(new AndroidLoggingFacility());
		locationManager = (LocationManager)getSystemService(Context.LOCATION_SERVICE);
		notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
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
		SharedPrefs.writeString(sp, LOCATION_LAST_STATUS, "");
		refreshHomeScreen();
		agentActive = sp.getBoolean("global.activate", false);
		notifyToast = sp.getBoolean("notification.toast", true);
		notifyIcon = sp.getBoolean("notification.icon", true);
		notificationType = Integer.parseInt(sp.getString("notification.status", "0"));
		connectionServer = sp.getString("connection.server", "");
		connectionPort = SafeParser.parseInt(sp.getString("connection.port", "4747"), 4747);
		connectionLogin = sp.getString("connection.login", "");
		connectionPassword = sp.getString("connection.password", "");
		connectionEncrypt = sp.getBoolean("connection.encrypt", false);
		connectionInterval = SafeParser.parseInt(sp.getString("connection.interval", "15"), 15) * 60 * 1000;
		schedulerDaily = sp.getBoolean("scheduler.daily.enable", false);
		locationStrategy = SafeParser.parseInt(sp.getString("location.strategy", "0"), 0);
		locationInterval = SafeParser.parseInt(sp.getString("location.interval", "30"), 30) * 60 * 1000;
		locationDuration = SafeParser.parseInt(sp.getString("location.duration", "2"), 2) * 60 * 1000;
		locationForce = sp.getBoolean("location.force", false);
		if (locationHandler != null)
			locationHandler.removeCallbacks(locationTask);
		if (locationManager != null)
		{
			if (agentActive)
			{
				if (locationHandler == null)
					locationHandler = new Handler(getMainLooper());
				if (locationHandler != null)
				{
					AgentConnectorService.gettingNewLocation = false;
					locationProvider = "";
					locationHandler.post(locationTask);
				}
			}
			else
			{
				locationManager.removeUpdates(AgentConnectorService.this);
			}
		}
	}

	/**
	 * Shutdown background service
	 */
	public void shutdown()
	{
		SharedPrefs.writeString(sp, LOCATION_LAST_STATUS, "");
		cancelConnectionSchedule();
		unregisterReceiver(receiver);
		stopSelf();
	}

	/**
	 * Show status notification (toast and icon, depending on settings)
	 * 
	 * @param status	connection status
	 * @param extra	extra text to add at the end of the toast/status message
	 */
	public void notification(ConnectionStatus status, String extra)
	{
		connectionStatus = status;
		String text = "";
		int icon = -1;
		switch (status)
		{
			case CS_CONNECTED:
				if (notificationType == NOTIFY_STATUS_ON_CONNECT || notificationType == NOTIFY_STATUS_ALWAYS)
				{
					icon = R.drawable.ic_stat_connected;
				}
				text = getString(R.string.notify_connected, extra);
				break;
			case CS_ERROR:
				if (notificationType == NOTIFY_STATUS_ON_DISCONNECT || notificationType == NOTIFY_STATUS_ALWAYS)
				{
					icon = R.drawable.ic_stat_disconnected;
				}
				text = getString(R.string.notify_connection_failed, extra);
				break;
			case CS_DISCONNECTED:
			case CS_NOCONNECTION:
			case CS_INPROGRESS:
			case CS_ALREADYCONNECTED:
			default:
				return;
		}
		if (notifyToast)
			showToast(text);
		if (icon == -1)
			notificationManager.cancel(NOTIFY_STATUS);
		else if (notifyIcon)
			showIcon(text, icon);
	}

	/**
	 * Show icon notification in status bar
	 * 
	 * @param text	notification text
	 * @param icon	icon to show
	 */
	public void showIcon(String text, int icon)
	{
		Intent notifyIntent = new Intent(getApplicationContext(), HomeScreen.class);
		PendingIntent intent = PendingIntent.getActivity(getApplicationContext(), 0, notifyIntent, PendingIntent.FLAG_UPDATE_CURRENT);
		NotificationCompat.Builder nb = new NotificationCompat.Builder(getApplicationContext())
				.setSmallIcon(icon)
				.setWhen(System.currentTimeMillis())
				.setDefaults(Notification.DEFAULT_LIGHTS)
				.setContentText(text)
				.setContentTitle(getString(R.string.app_name))
				.setContentIntent(intent);
		notificationManager.notify(NOTIFY_STATUS,
				new NotificationCompat.BigTextStyle(nb)
						.bigText(text)
						.build());
	}

	/**
	 * Reconnect to server.
	 * 
	 * @param force if set to true forces reconnection bypassing the scheduler
	 */
	public void reconnect(boolean force)
	{
		if (agentActive && (force || isConnectionScheduleExpired()) &&
				connectionStatus != ConnectionStatus.CS_INPROGRESS &&
				connectionStatus != ConnectionStatus.CS_CONNECTED)
		{
			new PushDataTask(connectionServer, connectionPort,
					DeviceInfoHelper.getDeviceId(getApplicationContext()),
					connectionLogin, connectionPassword, connectionEncrypt).execute();
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
	 * Show toast with given text
	 * 
	 * @param text message text
	 */
	public void showToast(final String text)
	{
		if (uiThreadHandler != null)
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
	 * Get the next schedule based on daily interval, if set
	 * 
	 * @param interval	expected schedule in milliseconds
	 * @return new schedule in milliseconds adjusted as necessary
	 */
	private long getNextSchedule(int interval)
	{
		if (!schedulerDaily)
			return interval;
		Calendar cal = Calendar.getInstance(); // get a Calendar object with current time
		long now = cal.getTimeInMillis();
		int on = getMinutes("scheduler.daily.on");
		int off = getMinutes("scheduler.daily.off");
		if (off < on)
			off += ONE_DAY_MINUTES; // Next day!
		Calendar calOn = (Calendar)cal.clone();
		setDayOffset(calOn, on);
		Calendar calOff = (Calendar)cal.clone();
		setDayOffset(calOff, off);
		cal.add(Calendar.MILLISECOND, interval);
		if (cal.before(calOn))
		{
			cal = (Calendar)calOn.clone();
			Log.i(TAG, "Rescheduled for daily interval (before 'on')");
		}
		else if (cal.after(calOff))
		{
			cal = (Calendar)calOn.clone();
			setDayOffset(cal, on + ONE_DAY_MINUTES); // Move to the next activation of the excluded range
			Log.i(TAG, "Rescheduled for daily interval (after 'off')");
		}
		Log.i(TAG, "Next schedule in " + (cal.getTimeInMillis() - now) / 1000 + " seconds");
		return cal.getTimeInMillis() - now;
	}

	/**
	 * Check for expired pending connection schedule
	 */
	private boolean isConnectionScheduleExpired()
	{
		Calendar cal = Calendar.getInstance(); // get a Calendar object with current time
		return cal.getTimeInMillis() > sp.getLong(SCHEDULER_NEXT_ACTIVATION, 0);
	}

	/**
	 * Set a connection schedule
	 * 
	 * @param milliseconds	time for new schedule
	 */
	private void setConnectionSchedule(long milliseconds)
	{
		Log.i(TAG, "setSchedule to: " + TimeHelper.getTimeString(milliseconds));
		Intent intent = new Intent(this, AlarmIntentReceiver.class);
		PendingIntent sender = PendingIntent.getBroadcast(this, NETXMS_REQUEST_CODE, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		((AlarmManager)getSystemService(ALARM_SERVICE)).set(AlarmManager.RTC_WAKEUP, milliseconds, sender);
		SharedPrefs.writeLong(sp, SCHEDULER_NEXT_ACTIVATION, milliseconds);
	}

	/**
	 * Cancel a pending connection schedule (if any)
	 */
	private void cancelConnectionSchedule()
	{
		Log.i(TAG, "cancelSchedule");
		Intent intent = new Intent(this, AlarmIntentReceiver.class);
		PendingIntent sender = PendingIntent.getBroadcast(this, NETXMS_REQUEST_CODE, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		((AlarmManager)getSystemService(ALARM_SERVICE)).cancel(sender);
		SharedPrefs.writeLong(sp, SCHEDULER_NEXT_ACTIVATION, 0);
	}

	/**
	 * Get flags. Currently not used.
	 */
	private int getFlags()
	{
		return 0;
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
			if (!sp.getBoolean(PERMISSIONS_GRANTED, false))
			{
				Log.d(TAG, "PushDataTask.doInBackground: necessary permissions not granted!");
				return false;
			}
			if (server.isEmpty() || login.isEmpty())
			{
				Log.d(TAG, "PushDataTask.doInBackground: server or login are empty!");
				return false;
			}
			Log.d(TAG, "PushDataTask.doInBackground: reconnecting...");
			notification(ConnectionStatus.CS_INPROGRESS, "");
			if (NetHelper.isInternetOn(getApplicationContext()))
			{
				Session session = new Session(server, port, deviceId, login, password, encrypt);
				try
				{
					session.connect();
					Log.v(TAG, "PushDataTask.doInBackground: connected");
					notification(ConnectionStatus.CS_CONNECTED, getString(R.string.notify_push_data));
					if (sendDeviceSystemInfo)
					{
						Log.v(TAG, "PushDataTask.doInBackground: sending DeviceSystemInfo");
						session.reportDeviceSystemInfo(DeviceInfoHelper.getManufacturer(), DeviceInfoHelper.getModel(),
								DeviceInfoHelper.getOSName(), DeviceInfoHelper.getRelease(), DeviceInfoHelper.getSerial(),
								DeviceInfoHelper.getUser(getApplicationContext()));
						sendDeviceSystemInfo = false;
					}
					session.reportDeviceStatus(NetHelper.getInetAddress(), getGeoLocation(), getFlags(),
							DeviceInfoHelper.getBatteryLevel(getApplicationContext()));
					session.disconnect();
					Log.v(TAG, "PushDataTask.doInBackground: data transfer completed");
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
				notification(ConnectionStatus.CS_DISCONNECTED, "");
			}
			else
			{
				Log.d(TAG, "PushDataTask.onPostExecute: error: " + connMsg);
				notification(ConnectionStatus.CS_ERROR, connMsg);
			}
			SharedPrefs.writeLong(sp, SCHEDULER_LAST_ACTIVATION, Calendar.getInstance().getTimeInMillis());
			SharedPrefs.writeString(sp, SCHEDULER_LAST_ACTIVATION_MSG, connMsg);
			if (agentActive)
				setConnectionSchedule(Calendar.getInstance().getTimeInMillis() + getNextSchedule(connectionInterval));
			else
				cancelConnectionSchedule();
			refreshHomeScreen();
		}
	}

	/**
	 * Convert location provider from Android type to NetXMS type.
	 * 
	 * @return Provider type
	 */
	private int getProviderType(String provider)
	{
		if (provider.compareTo(LocationManager.GPS_PROVIDER) == 0)
			return GeoLocation.GPS;
		else if (provider.compareTo(LocationManager.NETWORK_PROVIDER) == 0)
			return GeoLocation.NETWORK;
		return GeoLocation.UNSET;
	}

	/**
	 * Get last known geolocation (depending on strategy used it could be very old).
	 * If no provider available, try to get a last position from any available provider
	 * identified by the system using the ACCURACY_COARSE criteria.
	 * 
	 * @return Last known location or null if not known
	 */
	private GeoLocation getGeoLocation()
	{
		if (locationManager != null)
		{
			Location location = null;
			if (locationProvider.length() > 0) // Did we get an updated position?
			{
                if (ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED)
    				location = locationManager.getLastKnownLocation(locationProvider);
			}
			else
			{	// Try to get it using the best provide0r available
				Criteria criteria = new Criteria();
				if (criteria != null)
				{
					criteria.setAccuracy(Criteria.ACCURACY_COARSE);
					String bestProvider = locationManager.getBestProvider(criteria, true);
					if (bestProvider != null)
                        if (ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED)
                            location = locationManager.getLastKnownLocation(bestProvider);
				}
			}
			
			// getLastKnownLocation may return fake location at 0,0 if location is not known yet
			if ((location != null) && ((location.getLatitude() != 0.0) || (location.getLongitude() != 0.0)))
			{
				String locStatus = getString(R.string.info_location_good,
						TimeHelper.getTimeString(location.getTime()),
						location.getProvider(),
						Float.toString((float)location.getLatitude()),
						Float.toString((float)location.getLongitude()),
						Float.toString(location.getAccuracy()));
				Log.i(TAG, locStatus);
				SharedPrefs.writeString(sp, LOCATION_LAST_STATUS, locStatus);
				return new GeoLocation(location.getLatitude(),
						location.getLongitude(),
						getProviderType(location.getProvider()),
						(int)location.getAccuracy(),
						new Date(location.getTime()));
			}
		}
		return null;
	}

	/**
	 * Get list of enabled location provider based on selected strategy
	 * 
	 * @param	strategy: provider location strategy
	 * @return	List of enabled provider based on selected strategy
	 */
	private List<String> getLocationProviderList(int strategy)
	{
		List<String> providerList = new ArrayList<String>(0);
		providerList.clear();
		switch (strategy)
		{
			case STRATEGY_NET_ONLY:
				if (locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
					providerList.add(LocationManager.NETWORK_PROVIDER);
				break;
			case STRATEGY_GPS_ONLY:
				if (locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
					providerList.add(LocationManager.GPS_PROVIDER);
				break;
			case STRATEGY_NET_AND_GPS:
				if (locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
					providerList.add(LocationManager.NETWORK_PROVIDER);
				if (locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
					providerList.add(LocationManager.GPS_PROVIDER);
				break;
		}
		return providerList;
	}

	/**
	 * Internal handler to get new location based on location strategy set
	 */
	private final Runnable locationTask = new Runnable() {
		@Override
		public void run()
		{
			locationHandler.removeCallbacks(locationTask);
			if (AgentConnectorService.gettingNewLocation)
			{
				AgentConnectorService.gettingNewLocation = false;
				locationManager.removeUpdates(AgentConnectorService.this);
				locationHandler.postDelayed(locationTask, getNextSchedule(locationInterval));
				String locStatus = getString(R.string.info_location_timeout, allowedProviders);
				Log.i(TAG, locStatus);
				SharedPrefs.writeString(sp, LOCATION_LAST_STATUS, locStatus);
				refreshHomeScreen();
			}
			else if (locationForce)
			{
				locationProvider = "";
				String locStatus = getString(R.string.info_location_no_provider);
				List<String> providerList = getLocationProviderList(locationStrategy);
				if (providerList.size() > 0)
				{
					allowedProviders = "";
					for(int i = 0; i < providerList.size(); i++)
					{
						allowedProviders += (i > 0 ? ", " : "") + providerList.get(i);
                        if (ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED)
    						locationManager.requestLocationUpdates(providerList.get(i), 0, 0, AgentConnectorService.this); // 0, 0 to have it ASAP
					}
					AgentConnectorService.gettingNewLocation = true;
					locationHandler.postDelayed(locationTask, getNextSchedule(locationDuration));
					locStatus = getString(R.string.info_location_acquiring, allowedProviders);
				}
				else
				{
					locationHandler.postDelayed(locationTask, getNextSchedule(locationInterval));
				}
				Log.i(TAG, locStatus);
				SharedPrefs.writeString(sp, LOCATION_LAST_STATUS, locStatus);
				refreshHomeScreen();
			}
			else
			{
				if (ContextCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED)
					locationManager.requestLocationUpdates(LocationManager.PASSIVE_PROVIDER, 0, 0, AgentConnectorService.this); // 0, 0 to have it ASAP
			}
		}
	};

	/* (non-Javadoc)
	 * @see android.location.LocationListener#onLocationChanged(android.location.Location)
	 */
	@Override
	public void onLocationChanged(Location location)
	{
		String locStatus = getString(R.string.info_location_good,
				TimeHelper.getTimeString(location.getTime()),
				locationProvider = location.getProvider(),
				Float.toString((float)location.getLatitude()),
				Float.toString((float)location.getLongitude()),
				Float.toString(location.getAccuracy()));
		Log.i(TAG, locStatus);
		SharedPrefs.writeString(sp, LOCATION_LAST_STATUS, locStatus);
		locationManager.removeUpdates(AgentConnectorService.this);
		gettingNewLocation = false;
		locationHandler.removeCallbacks(locationTask);
		locationHandler.postDelayed(locationTask, locationInterval);
		refreshHomeScreen();
	}

	/* (non-Javadoc)
	 * @see android.location.LocationListener#onProviderDisabled(java.lang.String)
	 */
	@Override
	public void onProviderDisabled(String provider)
	{
		Log.d(TAG, "onProviderDisabled: " + provider);
	}

	/* (non-Javadoc)
	 * @see android.location.LocationListener#onProviderEnabled(java.lang.String)
	 */
	@Override
	public void onProviderEnabled(String provider)
	{
		Log.d(TAG, "onProviderEnabled: " + provider);
	}

	/* (non-Javadoc)
	 * @see android.location.LocationListener#onStatusChanged(java.lang.String, int, android.os.Bundle)
	 */
	@Override
	public void onStatusChanged(String provider, int status, Bundle extras)
	{
		Log.d(TAG, "onStatusChanged: " + provider + " status: " + status);
	}
}
