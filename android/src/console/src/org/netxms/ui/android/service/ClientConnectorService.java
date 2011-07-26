/**
 * 
 */
package org.netxms.ui.android.service;

import java.io.IOException;
import java.util.Map;

import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCException;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.AlarmBrowser;
import org.netxms.ui.android.main.HomeScreen;
import org.netxms.ui.android.service.tasks.ConnectTask;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.IBinder;
import android.preference.PreferenceManager;

/**
 * Background communication service for NetXMS client.
 *
 */
public class ClientConnectorService extends Service implements SessionListener
{
	public static final String ACTION_RECONNECT = "org.netxms.ui.android.ACTION_RECONNECT";
	
	private static final int NOTIFY_CONN_STATUS = 1;
	private static final int NOTIFY_ALARM = 1;
	
	private String mutex = "MUTEX";
	private Binder binder = new ClientConnectorBinder();
	private NotificationManager notificationManager;
	private NXCSession session = null;
	private boolean connectionInProgress = false;
	private Map<Long, Alarm> alarms = null;
	private AlarmBrowser alarmBrowser = null;
	
   /**
    * Class for clients to access.  Because we know this service always
    * runs in the same process as its clients, we don't need to deal with
    * IPC.
    */
   public class ClientConnectorBinder extends Binder 
   {
   	public ClientConnectorService getService() 
      {
   		return ClientConnectorService.this;
      }
   }

   /* (non-Javadoc)
	 * @see android.app.Service#onCreate()
	 */
	@Override
	public void onCreate()
	{
		super.onCreate();
		
		notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		showNotification(1, "NetXMS service started");

		BroadcastReceiver receiver = new BroadcastReceiver() {
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

	/* (non-Javadoc)
	 * @see android.app.Service#onStartCommand(android.content.Intent, int, int)
	 */
	@Override
	public int onStartCommand(Intent intent, int flags, int startId)
	{
		if ((intent.getAction() != null) && intent.getAction().equals(ACTION_RECONNECT))
			reconnect();
		return super.onStartCommand(intent, flags, startId);
	}

	/* (non-Javadoc)
	 * @see android.app.Service#onBind(android.content.Intent)
	 */
	@Override
	public IBinder onBind(Intent intent)
	{
		return binder;
	}

	/* (non-Javadoc)
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
	 * @param text
	 */
	public void showNotification(int id, String text)
	{
		Notification n = new Notification(android.R.drawable.stat_notify_sdcard, text, System.currentTimeMillis());
		n.defaults = Notification.DEFAULT_ALL;
		
		Intent notifyIntent = new Intent(getApplicationContext(), HomeScreen.class);
		PendingIntent intent = PendingIntent.getActivity(getApplicationContext(), 0, notifyIntent, Intent.FLAG_ACTIVITY_NEW_TASK);
		n.setLatestEventInfo(getApplicationContext(), getString(R.string.notification_title), text, intent);
		
		notificationManager.notify(id, n);
	}
	
	/**
	 * Hide notification
	 * 
	 * @param id notification id
	 */
	private void hideNotification(int id)
	{
		notificationManager.cancel(id);
	}
	
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
				SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
				new ConnectTask(this).execute(sp.getString("connection.server", ""), sp.getString("connection.login", ""), sp.getString("connection.password", ""));
			}
		}
	}
	
	/**
	 * Called by connect task after successful connection
	 * 
	 * @param session new session object
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
	 * Called by connect task or session notification listener after
	 * unsuccessful connection or disconnect
	 */
	public void onDisconnect()
	{
		synchronized(mutex)
		{
			if (session != null)
			{
				session.removeListener(this);
				hideNotification(NOTIFY_ALARM);
				showNotification(NOTIFY_CONN_STATUS, getString(R.string.notify_disconnected));
			}
			connectionInProgress = false;
			session = null;
			alarms = null;
		}
	}
	
	/**
	 * Process alarm change
	 * @param alarm
	 */
	private void processAlarmChange(Alarm alarm)
	{
		synchronized(mutex)
		{
			if (alarms != null)
			{
				alarms.put(alarm.getId(), alarm);
				showNotification(NOTIFY_ALARM, alarm.getMessage());
				long[] list = {alarm.getSourceObjectId()};
				try {
					session.syncMissingObjects(list,false);			
				} 
				catch (NXCException e) {} catch (IOException e) {}
			}
			if (this.alarmBrowser != null)
			{
				alarmBrowser.runOnUiThread(new Runnable(){ public void run() { alarmBrowser.refreshList(); } });
			}
		}
	}

	/**
	 * Process alarm change
	 * @param alarm
	 */
	private void processAlarmDelete(long id)
	{
		synchronized(mutex)
		{
			if (alarms != null)
			{
				alarms.remove(id);
			}
			if (this.alarmBrowser != null)
			{
				alarmBrowser.runOnUiThread(new Runnable(){ public void run() { alarmBrowser.refreshList(); } });
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(SessionNotification n)
	{
		switch(n.getCode())
		{
			case SessionNotification.CONNECTION_BROKEN:
			case SessionNotification.SERVER_SHUTDOWN:
				onDisconnect();
			case NXCNotification.NEW_ALARM:
			case NXCNotification.ALARM_CHANGED:
				processAlarmChange((Alarm)n.getObject());
				break;
			case NXCNotification.ALARM_DELETED:
			case NXCNotification.ALARM_TERMINATED:
				processAlarmDelete(((Alarm)n.getObject()).getId());
				break;
			default:
				break;
		}
	}
	
	/**
	 * Get list of active alarms
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
	
	public GenericObject findObjectById(long id) 
	{
		GenericObject obj = session.findObjectById(id);
		// if we don't have object - probably we never synced it (initial alarm viewer run?)
		// try to retreive it
		if (obj == null)
		{
			long[] list = {id};
			try {
				session.syncMissingObjects(list,false);
				obj = session.findObjectById(id);
			} 
			catch (NXCException e) {} catch (IOException e) {}
		}
		return obj;
	}
	
	public void TeminateAlarm(long id)
	{
		try {
			session.terminateAlarm(id);
		} catch (NXCException e) {}
		  catch (IOException e) {}
	}
	
	public void registerAlarmBrowser(AlarmBrowser browser)
	{
		this.alarmBrowser=browser;
	}
}
