/**
 * 
 */
package org.netxms.ui.android.service;

import java.util.List;
import java.util.Map;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.base.Logger;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.activities.AlarmBrowser;
import org.netxms.ui.android.main.activities.HomeScreen;
import org.netxms.ui.android.main.activities.NodeBrowser;
import org.netxms.ui.android.main.activities.GraphBrowser;
import org.netxms.ui.android.service.helpers.AndroidLoggingFacility;
import org.netxms.ui.android.service.tasks.ConnectTask;
import org.netxms.ui.android.service.tasks.ExecActionTask;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Toast;

/**
 * Background communication service for NetXMS client.
 * 
 */
public class ClientConnectorService extends Service implements SessionListener
{
	public static final String ACTION_RECONNECT = "org.netxms.ui.android.ACTION_RECONNECT";

	private static final int NOTIFY_ALARM = 1;
	private static final String TAG = "nxclient.ClientConnectorService";

	private String mutex = "MUTEX";
	private Binder binder = new ClientConnectorBinder();
	private Handler uiThreadHandler;
	private NotificationManager notificationManager;
	private NXCSession session = null;
	private boolean connectionInProgress = false;
	private String connectionStatus = "disconnected";
	private Map<Long, Alarm> alarms = null;
	private HomeScreen homeScreen = null;
	private AlarmBrowser alarmBrowser = null;
	private NodeBrowser nodeBrowser = null;
	private GraphBrowser graphBrowser = null;

	private List<ObjectTool> objectTools = null;

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

		showToast("NetXMS service started");

		BroadcastReceiver receiver = new BroadcastReceiver()
		{
			@Override
			public void onReceive(Context context, Intent intent)
			{
				Intent i = new Intent(context, ClientConnectorService.class);
				i.setAction(ACTION_RECONNECT);
				context.startService(i);
			}
		};
		registerReceiver(receiver, new IntentFilter(Intent.ACTION_TIME_TICK));
		
		reconnect();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Service#onStartCommand(android.content.Intent, int, int)
	 */
	@Override
	public int onStartCommand(Intent intent, int flags, int startId)
	{
		if ((intent != null) && (intent.getAction() != null) && intent.getAction().equals(ACTION_RECONNECT))
			reconnect();
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
	 * Show notification
	 * 
	 * @param severity
	 * @param text
	 */
	public void showNotification(int severity, String text)
	{
		Notification n = new Notification(GetAlarmIcon(severity), text, System.currentTimeMillis());
		n.defaults = Notification.DEFAULT_LIGHTS;

		Intent notifyIntent = new Intent(getApplicationContext(), AlarmBrowser.class);
		PendingIntent intent = PendingIntent.getActivity(getApplicationContext(), 0, notifyIntent, Intent.FLAG_ACTIVITY_NEW_TASK);
		n.setLatestEventInfo(getApplicationContext(), getString(R.string.notification_title), text, intent);
		final String sound = GetAlarmSound(severity);
		if ((sound != null) && (sound.length() > 0))
			n.sound = Uri.parse(sound);

		notificationManager.notify(NOTIFY_ALARM, n);
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
	 * Reconnect to server if needed
	 */
	private void reconnect()
	{
		synchronized(mutex)
		{
			if ((session == null) && !connectionInProgress)
			{
				connectionInProgress = true;
				setConnectionStatus(getString(R.string.notify_connecting));
				SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
				new ConnectTask(this).execute(sp.getString("connection.server", ""), sp.getString("connection.login", ""),
						sp.getString("connection.password", ""));
			}
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
		synchronized(mutex)
		{
			connectionInProgress = false;
			this.session = session;
			this.alarms = alarms;
			session.addListener(this);
		}
	}

	/**
	 * Called by connect task or session notification listener after unsuccessful
	 * connection or disconnect
	 */
	public void onDisconnect()
	{
		synchronized(mutex)
		{
			if (session != null)
			{
				session.removeListener(this);
				hideNotification(NOTIFY_ALARM);
				showToast(getString(R.string.notify_disconnected));
			}
			connectionInProgress = false;
			session = null;
			alarms = null;
		}
	}

	/**
	 * Process alarm change
	 * 
	 * @param alarm
	 */
	private void processAlarmChange(Alarm alarm)
	{
		synchronized(mutex)
		{
			if (alarms != null)
				alarms.put(alarm.getId(), alarm);
		}
		
		syncObjectIfMissing(alarm.getSourceObjectId());

		if (alarmBrowser != null)
		{
			alarmBrowser.runOnUiThread(new Runnable()
			{
				public void run()
				{
					alarmBrowser.refreshList();
				}
			});
		}

		GenericObject object = session.findObjectById(alarm.getSourceObjectId());
		showNotification(alarm.getCurrentSeverity(), ((object != null) ? object.getObjectName() : getString(R.string.node_unknown)) + ": " + alarm.getMessage());
	}

	/**
	 * Process alarm change
	 * 
	 * @param alarm
	 */
	private void processAlarmDelete(long id)
	{
		synchronized(mutex)
		{
			if (alarms != null)
				alarms.remove(id);
		}

		if (alarmBrowser != null)
		{
			alarmBrowser.runOnUiThread(new Runnable()
			{
				public void run()
				{
					alarmBrowser.refreshList();
				}
			});
		}
	}
	
	/**
	 * Synchronize information about specific object in background
	 * 
	 * @param objectId object ID
	 */
	private void doBackgroundObjectSync(final long objectId)
	{
		new Thread("Background object sync") {
			@Override
			public void run()
			{
				try
				{
					session.syncObjectSet(new long[] { objectId }, false, NXCSession.OBJECT_SYNC_NOTIFY);
				}
				catch(Exception e)
				{
					Log.d(TAG, "Exception in doBackgroundObjectSync", e);
				}
			}
		}.start();
	}
	
	/**
	 * Sync object with given ID if it is missing
	 * @param objectId
	 */
	private void syncObjectIfMissing(final long objectId)
	{
		if ((session != null) && (session.findObjectById(objectId) == null))
		{
			doBackgroundObjectSync(objectId);
		}
	}

	/**
	 * @param objectId
	 * @return
	 */
	public GenericObject findObjectById(long objectId)
	{
		// we can't search without active session
		if (session == null)
			return null;

		GenericObject object = session.findObjectById(objectId);
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
		synchronized(mutex)
		{
			if (this.alarmBrowser != null)
			{
				alarmBrowser.runOnUiThread(new Runnable()
				{
					public void run()
					{
						alarmBrowser.refreshList();
					}
				});
			}
			if (this.nodeBrowser != null)
			{
				nodeBrowser.runOnUiThread(new Runnable()
				{
					public void run()
					{
						nodeBrowser.refreshList();
					}
				});
			}
			if (this.graphBrowser != null)
			{
				graphBrowser.runOnUiThread(new Runnable()
				{
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
		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
		switch (severity)
		{
			case 0:	// Normal
				return sp.getString("alarm.sound.normal", "");
			case 1:	// Warning
				return sp.getString("alarm.sound.warning", "");
			case 2:	// Minor
				return sp.getString("alarm.sound.minor", "");
			case 3:	// Major
				return sp.getString("alarm.sound.major", "");
			case 4:	// Critical
				return sp.getString("alarm.sound.critical", "");
		}
		return "";
	}

	/**
	 * Get alarm icon based on alarm severity
	 * 
	 * @param severity
	 */
	private int GetAlarmIcon(int severity)
	{
		switch (severity)
		{
			case 0:	// Normal
				return R.drawable.status_normal;
			case 1:	// Warning
				return R.drawable.status_warning;
			case 2:	// Minor
				return R.drawable.status_minor;
			case 3:	// Major
				return R.drawable.status_major;
			case 4:	// Critical
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
		switch(n.getCode())
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
		synchronized(mutex)
		{
			if (alarms != null)
				a = alarms.values().toArray(new Alarm[alarms.size()]);
			else
				a = new Alarm[0];
		}
		return a;
	}

	/**
	 * @param id
	 */
	public void acknowledgeAlarm(long id)
	{
		try
		{
			session.acknowledgeAlarm(id);
		}
		catch(Exception e)
		{
		}
	}

	/**
	 * @param id
	 */
	public void teminateAlarm(long id)
	{
		try
		{
			session.terminateAlarm(id);
		}
		catch(Exception e)
		{
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
		catch(Exception e)
		{
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
		catch(Exception e)
		{
			this.objectTools = null;
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
		this.alarmBrowser = browser;
	}

	/**
	 * @param browser
	 */
	public void registerNodeBrowser(NodeBrowser browser)
	{
		this.nodeBrowser = browser;
	}

	/**
	 * @param browser
	 */
	public void registerGraphBrowser(GraphBrowser browser)
	{
		this.graphBrowser = browser;
	}

	/**
	 * @return the connectionStatus
	 */
	public String getConnectionStatus()
	{
		return connectionStatus;
	}

	/**
	 * @param connectionStatus
	 *           the connectionStatus to set
	 */
	public void setConnectionStatus(final String connectionStatus)
	{
		this.connectionStatus = connectionStatus;
		if (homeScreen != null)
		{
			homeScreen.runOnUiThread(new Runnable() {
				@Override
				public void run()
				{
					homeScreen.setStatusText(connectionStatus);
				}
			});
		}
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
		uiThreadHandler.post(new Runnable() {
			@Override
			public void run()
			{
				Toast.makeText(getApplicationContext(), text, Toast.LENGTH_SHORT).show();
			}
		});
	}
}
