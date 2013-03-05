/**
 * 
 */
package org.netxms.ui.android.service;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.base.Logger;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.android.NXApplication;
import org.netxms.ui.android.R;
import org.netxms.ui.android.helpers.SafeParser;
import org.netxms.ui.android.main.activities.AlarmBrowser;
import org.netxms.ui.android.main.activities.DashboardBrowser;
import org.netxms.ui.android.main.activities.GraphBrowser;
import org.netxms.ui.android.main.activities.HomeScreen;
import org.netxms.ui.android.main.activities.NodeBrowser;
import org.netxms.ui.android.receivers.AlarmIntentReceiver;
import org.netxms.ui.android.service.helpers.AndroidLoggingFacility;
import org.netxms.ui.android.service.tasks.ConnectTask;
import org.netxms.ui.android.service.tasks.ExecActionTask;

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
import android.content.SharedPreferences.Editor;
import android.content.res.Resources;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.support.v4.app.NotificationCompat;
import android.support.v4.content.Loader;
import android.util.Log;
import android.widget.Toast;

/**
 * Background communication service for NetXMS client.
 * 
 * @author Victor Kirhenshtein
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class ClientConnectorService extends Service implements SessionListener
{
	public enum ConnectionStatus
	{
		CS_NOCONNECTION, CS_INPROGRESS, CS_ALREADYCONNECTED, CS_CONNECTED, CS_DISCONNECTED, CS_ERROR
	};

	public static final String ACTION_CONNECT = "org.netxms.ui.android.ACTION_CONNECT";
	public static final String ACTION_FORCE_CONNECT = "org.netxms.ui.android.ACTION_FORCE_CONNECT";
	public static final String ACTION_DISCONNECT = "org.netxms.ui.android.ACTION_DISCONNECT";
	public static final String ACTION_FORCE_DISCONNECT = "org.netxms.ui.android.ACTION_FORCE_DISCONNECT";
	public static final String ACTION_RESCHEDULE = "org.netxms.ui.android.ACTION_RESCHEDULE";
	public static final String ACTION_CONFIGURE = "org.netxms.ui.android.ACTION_CONFIGURE";
	private static final String TAG = "nxclient/ClientConnectorService";
	private static final String LASTALARM_KEY = "LastALarmIdNotified";

	private static final int NOTIFY_ALARM = 1;
	private static final int NOTIFY_STATUS = 2;
	private static final int NOTIFY_STATUS_ON_CONNECT = 1;
	private static final int NOTIFY_STATUS_ON_DISCONNECT = 2;
	private static final int NOTIFY_STATUS_ALWAYS = 3;
	private static final int ONE_DAY_MINUTES = 24 * 60;
	private static final int NETXMS_REQUEST_CODE = 123456;

	private final String mutex = "MUTEX";
	private final Binder binder = new ClientConnectorBinder();
	private Handler uiThreadHandler;
	private NotificationManager notificationManager;
	private NXCSession session = null;
	private ConnectionStatus connectionStatus = ConnectionStatus.CS_DISCONNECTED;
	private String connectionStatusText = "";
	private int connectionStatusColor = 0;
	private Map<Long, Alarm> alarms = null;
	private HomeScreen homeScreen = null;
	private AlarmBrowser alarmBrowser = null;
	private NodeBrowser nodeBrowser = null;
	private GraphBrowser graphBrowser = null;
	private Alarm unknownAlarm = null;
	private long lastAlarmIdNotified;
	private List<ObjectTool> objectTools = null;
	private BroadcastReceiver receiver = null;
	private DashboardBrowser dashboardBrowser;
	private SharedPreferences sp;
	private final List<Loader<Alarm[]>> alarmLoaders = new ArrayList<Loader<Alarm[]>>(0);
	private final List<Loader<DciValue[]>> dciValueLoaders = new ArrayList<Loader<DciValue[]>>(0);
	private final List<Loader<GenericObject>> genericObjectLoaders = new ArrayList<Loader<GenericObject>>(0);
	private final List<Loader<Set<GenericObject>>> genericObjectChildrenLoaders = new ArrayList<Loader<Set<GenericObject>>>(0);
	private String server = "";
	private int port = 4701;
	private String login = "";
	private String password = "";
	private boolean encrypt = false;
	private boolean enabled = false;
	private boolean notifyAlarm = false;
	private int notificationType = 0;
	private boolean notifyIcon = false;
	private boolean notifyToast = false;
	private int schedulerPostpone = 0;
	private int schedulerDuration = 0;
	private int schedulerInterval = 0;
	private boolean schedulerDailyEnabled = false;
	private int schedulerDailyOn = 0;
	private int schedulerDailyOff = 0;

	/**
	 * Class for clients to access. Because we know this service always runs in
	 * the same process as its clients, we don't need to deal with IPC.
	 */
	public class ClientConnectorBinder extends Binder
	{
		public ClientConnectorService getService()
		{
			return ClientConnectorService.this;
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

		Logger.setLoggingFacility(new AndroidLoggingFacility());
		uiThreadHandler = new Handler(getMainLooper());
		notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);

		showToast(getString(R.string.notify_started));

		sp = PreferenceManager.getDefaultSharedPreferences(this);
		lastAlarmIdNotified = sp.getInt(LASTALARM_KEY, 0);
		configure();

		receiver = new BroadcastReceiver()
		{
			@Override
			public void onReceive(Context context, Intent intent)
			{
				Intent i = new Intent(context, ClientConnectorService.class);
				i.setAction(ACTION_RESCHEDULE);
				context.startService(i);
			}
		};
		registerReceiver(receiver, new IntentFilter(Intent.ACTION_TIME_TICK));

		if (NXApplication.isActivityVisible())
			reconnect(false);
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
			if (intent.getAction().equals(ACTION_CONNECT))
				reconnect(false);
			else if (intent.getAction().equals(ACTION_FORCE_CONNECT))
				reconnect(true);
			else if (intent.getAction().equals(ACTION_DISCONNECT))
				disconnect(false);
			else if (intent.getAction().equals(ACTION_FORCE_DISCONNECT))
				disconnect(true);
			else if (intent.getAction().equals(ACTION_RESCHEDULE))
				if (NXApplication.isActivityVisible())
					reconnect(false);
				else
					disconnect(false);
			else if (intent.getAction().equals(ACTION_CONFIGURE))
				reconnect(configure());
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
	 * Shutdown background service
	 */
	public void shutdown()
	{
		cancelSchedule();
		clearNotifications();
		savePreferences();
		unregisterReceiver(receiver);
		stopSelf();
	}

	/**
	 * 
	 */
	public void savePreferences()
	{
		SharedPreferences.Editor editor = sp.edit();
		editor.putInt(LASTALARM_KEY, (int)lastAlarmIdNotified);
		editor.commit();
	}

	/**
	 * Show alarm notification
	 * 
	 * @param severity	notification severity (used to determine icon and sound)
	 * @param text	notification text
	 */
	public void alarmNotification(int severity, String text)
	{
		if (notifyAlarm)
		{
			Intent notifyIntent = new Intent(getApplicationContext(), AlarmBrowser.class);
			PendingIntent intent = PendingIntent.getActivity(getApplicationContext(), 0, notifyIntent, Intent.FLAG_ACTIVITY_NEW_TASK);
			NotificationCompat.Builder nb = new NotificationCompat.Builder(getApplicationContext())
					.setSmallIcon(getAlarmIcon(severity))
					.setWhen(System.currentTimeMillis())
					.setDefaults(Notification.DEFAULT_LIGHTS)
					.setContentText(text)
					.setContentTitle(getString(R.string.notification_title))
					.setContentIntent(intent);
			final String sound = GetAlarmSound(severity);
			if ((sound != null) && (sound.length() > 0))
				nb.setSound(Uri.parse(sound));
			notificationManager.notify(NOTIFY_ALARM, nb.build());
		}
		else
		{
			Ringtone r = RingtoneManager.getRingtone(getApplicationContext(), Uri.parse(GetAlarmSound(severity)));
			if (r != null)
				r.play();
		}
	}
	/**
	 * Show status notification
	 * 
	 * @param status	connection status
	 * @param extra	extra text to add at the end of the toast
	 */
	public void statusNotification(ConnectionStatus status, String extra)
	{
		int icon = -1;
		String text = "";
		switch (status)
		{
			case CS_CONNECTED:
				if (notificationType == NOTIFY_STATUS_ON_CONNECT || notificationType == NOTIFY_STATUS_ALWAYS)
				{
					icon = R.drawable.ic_stat_connected;
				}
				text = getString(R.string.notify_connected, extra);
				break;
			case CS_DISCONNECTED:
				if (notificationType == NOTIFY_STATUS_ON_DISCONNECT || notificationType == NOTIFY_STATUS_ALWAYS)
				{
					icon = R.drawable.ic_stat_disconnected;
				}
				text = getString(R.string.notify_disconnected) + getNextConnectionRetry();
				break;
			case CS_ERROR:
				if (notificationType == NOTIFY_STATUS_ON_DISCONNECT || notificationType == NOTIFY_STATUS_ALWAYS)
				{
					icon = R.drawable.ic_stat_disconnected;
				}
				text = getString(R.string.notify_connection_failed, extra);
				break;
			case CS_NOCONNECTION:
			case CS_INPROGRESS:
			case CS_ALREADYCONNECTED:
			default:
				return;
		}
		if (icon == -1)
			hideNotification(NOTIFY_STATUS);
		else
		{
			if (notifyToast)
				showToast(text);
			if (notifyIcon)
			{
				Intent notifyIntent = new Intent(getApplicationContext(), HomeScreen.class);
				PendingIntent intent = PendingIntent.getActivity(getApplicationContext(), 0, notifyIntent, Intent.FLAG_ACTIVITY_NEW_TASK);
				NotificationCompat.Builder nb = new NotificationCompat.Builder(getApplicationContext())
						.setSmallIcon(icon)
						.setWhen(System.currentTimeMillis())
						.setAutoCancel(false)
						.setOnlyAlertOnce(true)
						.setOngoing(true)
						.setContentText(text)
						.setContentTitle(getString(R.string.notification_title))
						.setContentIntent(intent);
				notificationManager.notify(NOTIFY_STATUS, nb.build());
			}
		}
	}

	/**
	 * Hide notification
	 * 
	 * @param id
	 *           notification id
	 */
	private void hideNotification(int id)
	{
		notificationManager.cancel(id);
	}

	/**
	 * Clear all notifications
	 */
	public void clearNotifications()
	{
		notificationManager.cancelAll();
	}

	/**
	 * Configure service, notify callers if it is necessary to reconnect
	 * depending if a particular subset of parameters have been changed.
	 * 
	 * @return true If it is necessary to force a reconnection
	 */
	private boolean configure()
	{
		boolean needsToReconnect = enabled != sp.getBoolean("global.scheduler.enable", false) ||
				server != sp.getString("connection.server", "") ||
				port != SafeParser.parseInt(sp.getString("connection.port", "4701"), 4701) ||
				login != sp.getString("connection.login", "") ||
				password != sp.getString("connection.password", "") ||
				encrypt != sp.getBoolean("connection.encrypt", false) ||
				notifyAlarm != sp.getBoolean("global.notification.alarm", true) ||
				notificationType != Integer.parseInt(sp.getString("global.notification.status", "0")) ||
				notifyIcon != sp.getBoolean("global.notification.icon", false) ||
				notifyToast != sp.getBoolean("global.notification.toast", true) ||
				schedulerPostpone != Integer.parseInt(sp.getString("global.scheduler.postpone", "1")) ||
				schedulerDuration != Integer.parseInt(sp.getString("global.scheduler.duration", "1")) ||
				schedulerInterval != Integer.parseInt(sp.getString("global.scheduler.interval", "15")) ||
				schedulerDailyEnabled != sp.getBoolean("global.scheduler.daily.enable", false) ||
				schedulerDailyOn != getMinutes("global.scheduler.daily.on") ||
				schedulerDailyOff != getMinutes("global.scheduler.daily.off");

		enabled = sp.getBoolean("global.scheduler.enable", false);
		server = sp.getString("connection.server", "");
		port = SafeParser.parseInt(sp.getString("connection.port", "4701"), 4701);
		login = sp.getString("connection.login", "");
		password = sp.getString("connection.password", "");
		encrypt = sp.getBoolean("connection.encrypt", false);
		notifyAlarm = sp.getBoolean("global.notification.alarm", true);
		notificationType = Integer.parseInt(sp.getString("global.notification.status", "0"));
		notifyIcon = sp.getBoolean("global.notification.icon", false);
		notifyToast = sp.getBoolean("global.notification.toast", true);
		schedulerPostpone = Integer.parseInt(sp.getString("global.scheduler.postpone", "1"));
		schedulerDuration = Integer.parseInt(sp.getString("global.scheduler.duration", "1"));
		schedulerInterval = Integer.parseInt(sp.getString("global.scheduler.interval", "15"));
		schedulerDailyEnabled = sp.getBoolean("global.scheduler.daily.enable", false);
		schedulerDailyOn = getMinutes("global.scheduler.daily.on");
		schedulerDailyOff = getMinutes("global.scheduler.daily.off");

		return needsToReconnect;
	}

	/**
	 * Reconnect to server.
	 * 
	 * @param forceReconnect if set to true forces disconnection before connecting
	 */
	public void reconnect(boolean force)
	{
		if (force || (isScheduleExpired() || NXApplication.isActivityVisible()) &&
				connectionStatus != ConnectionStatus.CS_CONNECTED &&
				connectionStatus != ConnectionStatus.CS_ALREADYCONNECTED)
		{
			Log.i(TAG, "Reconnecting...");
			synchronized (mutex)
			{
				if (connectionStatus != ConnectionStatus.CS_INPROGRESS)
				{
					setConnectionStatus(ConnectionStatus.CS_INPROGRESS, "");
					statusNotification(ConnectionStatus.CS_INPROGRESS, "");
					new ConnectTask(this).execute(server, port, login, password, encrypt, force);
				}
			}
		}
	}

	/**
	 * Disconnect from server. Only when scheduler is enabled and connected.
	 * 
	 */
	public void disconnect(boolean force)
	{
		if (force || enabled && !NXApplication.isActivityVisible() &&
				(connectionStatus == ConnectionStatus.CS_CONNECTED ||
				connectionStatus == ConnectionStatus.CS_ALREADYCONNECTED))
		{
			Log.i(TAG, "Disconnecting...");
			nullifySession();
			setConnectionStatus(ConnectionStatus.CS_DISCONNECTED, "");
			statusNotification(ConnectionStatus.CS_DISCONNECTED, "");
		}
	}

	/**
	 * Called by connect task after successful connection
	 * 
	 * @param session
	 *           new session object
	 */
	public void onConnect(NXCSession session, Map<Long, Alarm> alarms)
	{
		synchronized (mutex)
		{
			if (session != null)
			{
				schedule(ACTION_DISCONNECT);
				this.session = session;
				this.alarms = alarms;
				session.addListener(this);
				setConnectionStatus(ConnectionStatus.CS_CONNECTED, session.getServerAddress());
				statusNotification(ConnectionStatus.CS_CONNECTED, session.getServerAddress());
				if (alarms != null)
				{
					long id = -1;
					Alarm alarm = null;
					// Find the newest alarm received when we were offline
					for (Alarm itAlarm : alarms.values())
						if (itAlarm.getId() > id)
						{
							alarm = itAlarm;
							id = itAlarm.getId();
						}
					if (alarm != null && alarm.getId() > lastAlarmIdNotified)
						processAlarmChange(alarm);
				}
			}
		}
	}

	/**
	 * Called by connect task or session notification listener after unsuccessful
	 * connection or disconnect
	 */
	public void onDisconnect()
	{
		schedule(ACTION_CONNECT);
		nullifySession();
		setConnectionStatus(ConnectionStatus.CS_DISCONNECTED, "");
		statusNotification(ConnectionStatus.CS_DISCONNECTED, "");
	}

	/**
	 * Called by connect task on error during connection
	 */
	public void onError(String error)
	{
		nullifySession();
		setConnectionStatus(ConnectionStatus.CS_ERROR, error);
		statusNotification(ConnectionStatus.CS_ERROR, error);
	}

	/**
	 * Check for expired pending connection schedule
	 */
	private boolean isScheduleExpired()
	{
		if (enabled)
		{
			Calendar cal = Calendar.getInstance(); // get a Calendar object with current time
			return cal.getTimeInMillis() > sp.getLong("global.scheduler.next_activation", 0);
		}
		return true;
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
	public void schedule(String action)
	{
		Log.i(TAG, "Schedule: " + action);
		if (!enabled)
			cancelSchedule();
		else
		{
			Calendar cal = Calendar.getInstance(); // get a Calendar object with current time
			if (action == ACTION_RESCHEDULE)
				cal.add(Calendar.MINUTE, schedulerPostpone);
			if (action == ACTION_DISCONNECT)
				cal.add(Calendar.MINUTE, schedulerDuration);
			else if (!schedulerDailyEnabled)
				cal.add(Calendar.MINUTE, schedulerInterval);
			else
			{
				int on = schedulerDailyOn;
				int off = schedulerDailyOff;
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
			setSchedule(cal.getTimeInMillis(), action);
		}
	}

	/**
	 * Set a connection schedule
	 */
	private void setSchedule(long milliseconds, String action)
	{
		Intent intent = new Intent(this, AlarmIntentReceiver.class);
		intent.putExtra("action", action);
		PendingIntent sender = PendingIntent.getBroadcast(this, NETXMS_REQUEST_CODE, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		((AlarmManager)getSystemService(ALARM_SERVICE)).set(AlarmManager.RTC_WAKEUP, milliseconds, sender);
		// Update status
		long last = sp.getLong("global.scheduler.next_activation", 0);
		Editor e = sp.edit();
		e.putLong("global.scheduler.last_activation", last);
		e.putLong("global.scheduler.next_activation", milliseconds);
		e.commit();
	}

	/**
	 * Cancel a pending connection schedule (if any)
	 */
	public void cancelSchedule()
	{
		Intent intent = new Intent(this, AlarmIntentReceiver.class);
		PendingIntent sender = PendingIntent.getBroadcast(this, NETXMS_REQUEST_CODE, intent, PendingIntent.FLAG_UPDATE_CURRENT);
		((AlarmManager)getSystemService(ALARM_SERVICE)).cancel(sender);
		if (sp.getLong("global.scheduler.next_activation", 0) != 0)
		{
			Editor e = sp.edit();
			e.putLong("global.scheduler.next_activation", 0);
			e.commit();
		}
	}

	/**
	 * Release internal resources nullifying current session (if any)
	 */
	private void nullifySession()
	{
		synchronized (mutex)
		{
			if (session != null)
			{
				session.disconnect();
				session.removeListener(this);
				session = null;
			}
			alarms = null;
			unknownAlarm = null;
		}
	}

	/**
	 * Process alarm change
	 * 
	 * @param alarm
	 */
	private void processAlarmChange(Alarm alarm)
	{
		GenericObject object = findObjectById(alarm.getSourceObjectId());
		synchronized (mutex)
		{
			if (alarms != null)
			{
				lastAlarmIdNotified = alarm.getId();
				alarms.put(lastAlarmIdNotified, alarm);
			}
			unknownAlarm = object == null ? alarm : null;
			alarmNotification(alarm.getCurrentSeverity(), ((object != null) ? object.getObjectName() : getString(R.string.node_unknown)) + ": " + alarm.getMessage());
			refreshAlarmBrowser();
		}
	}

	/**
	 * Process alarm change
	 * 
	 * @param alarm
	 */
	private void processAlarmDelete(long id)
	{
		synchronized (mutex)
		{
			if (alarms != null)
				alarms.remove(id);
			if (lastAlarmIdNotified == id)
				hideNotification(NOTIFY_ALARM);
			refreshAlarmBrowser();
		}
	}

	/**
	 * Synchronize information about specific object in background
	 * 
	 * @param objectId object ID
	 */
	private void doBackgroundObjectSync(final long objectId)
	{
		new Thread("Background object sync")
		{
			@Override
			public void run()
			{
				try
				{
					session.syncObjectSet(new long[] { objectId }, false, NXCSession.OBJECT_SYNC_NOTIFY);
				}
				catch (Exception e)
				{
					Log.d(TAG, "Exception in doBackgroundObjectSync", e);
				}
			}
		}.start();
	}

	/**
	 * @param objectId
	 * @return
	 */
	public GenericObject findObjectById(long objectId)
	{
		return findObjectById(objectId, GenericObject.class);
	}

	/**
	 * Find object by ID with class filter
	 * 
	 * @param objectId
	 * @param classFilter
	 * @return
	 */
	public GenericObject findObjectById(long objectId, Class<? extends GenericObject> classFilter)
	{
		// we can't search without active session
		if (session == null)
			return null;

		GenericObject object = session.findObjectById(objectId, classFilter);
		// if we don't have object - probably we never synced it
		// request object synchronization in that case
		if (object == null)
		{
			doBackgroundObjectSync(objectId);
		}
		return object;
	}

	/**
	 * @param object
	 */
	private void processObjectUpdate(GenericObject object)
	{
		synchronized (mutex)
		{
			if (unknownAlarm != null && unknownAlarm.getSourceObjectId() == object.getObjectId()) // Update <Unknown> notification
			{
				alarmNotification(unknownAlarm.getCurrentSeverity(), object.getObjectName() + ": " + unknownAlarm.getMessage());
				unknownAlarm = null;
			}
			refreshHomeScreen();
			refreshAlarmBrowser();
			refreshNodeBrowser();
			refreshDashboardBrowser();
		}
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
					homeScreen.refreshActivityStatus();
				}
			});
		}
	}

	/**
	 * Refresh the alarms related activities
	 */
	private void refreshAlarmBrowser()
	{
		if (alarmBrowser != null)
		{
			alarmBrowser.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					alarmBrowser.refreshList();
				}
			});
		}
		if (homeScreen != null)
		{
			homeScreen.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					homeScreen.refreshPendingAlarms();
				}
			});
		}
		for (Loader<Alarm[]> l : alarmLoaders)
			l.forceLoad();
	}

	/**
	 * Refresh the node browser activity
	 */
	private void refreshNodeBrowser()
	{
		if (nodeBrowser != null)
		{
			nodeBrowser.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					nodeBrowser.refreshList();
				}
			});
		}
		for (Loader<DciValue[]> l : dciValueLoaders)
			l.forceLoad();
		for (Loader<GenericObject> l : genericObjectLoaders)
			l.forceLoad();
		for (Loader<Set<GenericObject>> l : genericObjectChildrenLoaders)
			l.forceLoad();
	}
	/**
	 * Refresh dashboard browser activity
	 */
	private void refreshDashboardBrowser()
	{
		if (dashboardBrowser != null)
		{
			dashboardBrowser.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					dashboardBrowser.refreshList();
				}
			});
		}
	}

	/**
	 * Process graph update event
	 */
	private void processGraphUpdate()
	{
		synchronized (mutex)
		{
			if (this.graphBrowser != null)
			{
				graphBrowser.runOnUiThread(new Runnable()
				{
					@Override
					public void run()
					{
						graphBrowser.refreshList();
					}
				});
			}
		}
	}

	/**
	 * Get alarm sound based on alarm severity
	 * 
	 * @param severity
	 */
	private String GetAlarmSound(int severity)
	{
		switch (severity)
		{
			case 0: // Normal
				return sp.getString("alarm.sound.normal", "");
			case 1: // Warning
				return sp.getString("alarm.sound.warning", "");
			case 2: // Minor
				return sp.getString("alarm.sound.minor", "");
			case 3: // Major
				return sp.getString("alarm.sound.major", "");
			case 4: // Critical
				return sp.getString("alarm.sound.critical", "");
		}
		return "";
	}

	/**
	 * Get alarm icon based on alarm severity
	 * 
	 * @param severity
	 */
	private int getAlarmIcon(int severity)
	{
		switch (severity)
		{
			case 0: // Normal
				return R.drawable.status_normal;
			case 1: // Warning
				return R.drawable.status_warning;
			case 2: // Minor
				return R.drawable.status_minor;
			case 3: // Major
				return R.drawable.status_major;
			case 4: // Critical
				return R.drawable.status_critical;
		}
		return android.R.drawable.stat_notify_sdcard;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api
	 * .client.SessionNotification)
	 */
	@Override
	public void notificationHandler(SessionNotification n)
	{
		switch (n.getCode())
		{
			case SessionNotification.CONNECTION_BROKEN:
			case SessionNotification.SERVER_SHUTDOWN:
				onDisconnect();
				break;
			case NXCNotification.NEW_ALARM:
			case NXCNotification.ALARM_CHANGED:
				processAlarmChange((Alarm)n.getObject());
				break;
			case NXCNotification.ALARM_DELETED:
			case NXCNotification.ALARM_TERMINATED:
				processAlarmDelete(((Alarm)n.getObject()).getId());
				break;
			case NXCNotification.OBJECT_CHANGED:
			case NXCNotification.OBJECT_SYNC_COMPLETED:
				processObjectUpdate((GenericObject)n.getObject());
				break;
			case NXCNotification.PREDEFINED_GRAPHS_CHANGED:
				processGraphUpdate();
				break;
			default:
				break;
		}
	}

	/**
	 * Get list of active alarms
	 * 
	 * @return list of active alarms
	 */
	public Alarm[] getAlarms()
	{
		Alarm[] a;
		synchronized (mutex)
		{
			if (alarms != null)
				a = alarms.values().toArray(new Alarm[alarms.size()]);
			else
				a = new Alarm[0];
		}
		return a;
	}

	/**
	 * @param ids
	 */
	public void acknowledgeAlarm(ArrayList<Long> ids, boolean sticky)
	{
		try
		{
			for (int i = 0; i < ids.size(); i++)
				session.acknowledgeAlarm(ids.get(i).longValue(), sticky);
		}
		catch (Exception e)
		{
			Log.d(TAG, "Exception while executing session.acknowledgeAlarm", e);
		}
	}

	/**
	 * @param ids
	 */
	public void resolveAlarm(ArrayList<Long> ids)
	{
		try
		{
			for (int i = 0; i < ids.size(); i++)
				session.resolveAlarm(ids.get(i).longValue());
		}
		catch (Exception e)
		{
			Log.d(TAG, "Exception while executing session.resolveAlarm", e);
		}
	}

	/**
	 * @param ids
	 */
	public void terminateAlarm(ArrayList<Long> ids)
	{
		try
		{
			for (int i = 0; i < ids.size(); i++)
				session.terminateAlarm(ids.get(i).longValue());
		}
		catch (Exception e)
		{
			Log.d(TAG, "Exception while executing session.terminateAlarm", e);
		}
	}

	/**
	 * @param id
	 * @param state
	 */
	public void setObjectMgmtState(long id, boolean state)
	{
		try
		{
			session.setObjectManaged(id, state);
		}
		catch (Exception e)
		{
			Log.d(TAG, "Exception while executing session.setObjectManaged", e);
		}
	}

	/**
	 * 
	 */
	public void loadTools()
	{
		try
		{
			this.objectTools = session.getObjectTools();
		}
		catch (Exception e)
		{
			this.objectTools = null;
			Log.d(TAG, "Exception while executing session.getObjectTools", e);
		}
	}

	/**
	 * Execute agent action. Communication with server will be done in separate worker thread.
	 * 
	 * @param objectId
	 * @param action
	 */
	public void executeAction(long objectId, String action)
	{
		new ExecActionTask().execute(new Object[] { session, objectId, action, this });
	}

	/**
	 * @return
	 */
	public List<ObjectTool> getTools()
	{
		return this.objectTools;
	}

	/**
	 * @param homeScreen
	 */
	public void registerHomeScreen(HomeScreen homeScreen)
	{
		this.homeScreen = homeScreen;
	}

	/**
	 * @param browser
	 */
	public void registerAlarmBrowser(AlarmBrowser browser)
	{
		alarmBrowser = browser;
	}

	/**
	 * @param browser
	 */
	public void registerNodeBrowser(NodeBrowser browser)
	{
		nodeBrowser = browser;
	}

	/**
	 * @param browser
	 */
	public void registerGraphBrowser(GraphBrowser browser)
	{
		graphBrowser = browser;
	}

	/**
	 * @param browser
	 */
	public void registerDashboardBrowser(DashboardBrowser browser)
	{
		dashboardBrowser = browser;
	}

	public void registerAlarmLoader(Loader<Alarm[]> loader)
	{
		if (!alarmLoaders.contains(this))
			alarmLoaders.add(loader);
	}

	public void unregisterAlarmLoader(Loader<Alarm[]> loader)
	{
		alarmLoaders.remove(loader);
	}

	public void registerDciValueLoader(Loader<DciValue[]> loader)
	{
		if (!dciValueLoaders.contains(this))
			dciValueLoaders.add(loader);
	}

	public void unregisterDciValueLoader(Loader<DciValue[]> loader)
	{
		dciValueLoaders.remove(loader);
	}

	public void registerGenericObjectLoader(Loader<GenericObject> loader)
	{
		if (!genericObjectLoaders.contains(this))
			genericObjectLoaders.add(loader);
	}

	public void unregisterGenericObjectLoader(Loader<GenericObject> loader)
	{
		genericObjectLoaders.remove(loader);
	}

	public void registerGenericObjectChildrenLoader(Loader<Set<GenericObject>> loader)
	{
		if (!genericObjectChildrenLoaders.contains(this))
			genericObjectChildrenLoaders.add(loader);
	}

	public void unregisterGenericObjectChildrenLoader(Loader<Set<GenericObject>> loader)
	{
		genericObjectChildrenLoaders.remove(loader);
	}

	/**
	 * @return the connectionStatus
	 */
	public ConnectionStatus getConnectionStatus()
	{
		return connectionStatus;
	}

	/**
	 * @return the connectionStatusText
	 */
	public String getConnectionStatusText()
	{
		return connectionStatusText;
	}

	/**
	 * @return the connectionStatusColor
	 */
	public int getConnectionStatusColor()
	{
		return connectionStatusColor;
	}

	/**
	 * @param connectionStatus
	 *           the connectionStatus to set
	 */
	public void setConnectionStatus(ConnectionStatus connectionStatus, String extra)
	{
		Resources r = getResources();
		this.connectionStatus = connectionStatus;
		switch (connectionStatus)
		{
			case CS_NOCONNECTION:
				connectionStatusText = getString(R.string.notify_no_connection);
				connectionStatusColor = r.getColor(R.color.notify_no_connection);
				break;
			case CS_INPROGRESS:
				connectionStatusText = getString(R.string.notify_connecting);
				connectionStatusColor = r.getColor(R.color.notify_connecting);
				break;
			case CS_ALREADYCONNECTED:
				connectionStatusText = getString(R.string.notify_connected, extra);
				connectionStatusColor = r.getColor(R.color.notify_connected);
				break;
			case CS_CONNECTED:
				connectionStatusText = getString(R.string.notify_connected, extra);
				connectionStatusColor = r.getColor(R.color.notify_connected);
				break;
			case CS_DISCONNECTED:
				connectionStatusText = getString(R.string.notify_disconnected) + getNextConnectionRetry();
				connectionStatusColor = r.getColor(R.color.notify_disconnected);
				break;
			case CS_ERROR:
				connectionStatusText = getString(R.string.notify_connection_failed, extra);
				connectionStatusColor = r.getColor(R.color.notify_connection_failed);
				break;
			default:
				break;
		}
		if (homeScreen != null)
		{
			homeScreen.runOnUiThread(new Runnable()
			{
				@Override
				public void run()
				{
					homeScreen.setStatusText(connectionStatusText, connectionStatusColor);
					homeScreen.refreshActivityStatus();
				}
			});
		}
	}

	public String getNextConnectionRetry()
	{
		long next = sp.getLong("global.scheduler.next_activation", 0);
		if (next != 0)
		{
			Calendar cal = Calendar.getInstance(); // get a Calendar object with current time
			if (cal.getTimeInMillis() < next)
			{
				cal.setTimeInMillis(next);
				return " " + getString(R.string.notify_next_connection_schedule, DateFormat.getDateTimeInstance().format(cal));
			}
		}
		return "";
	}

	/**
	 * @return the session
	 */
	public NXCSession getSession()
	{
		return session;
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
}
