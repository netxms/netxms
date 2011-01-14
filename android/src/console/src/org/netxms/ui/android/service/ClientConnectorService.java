/**
 * 
 */
package org.netxms.ui.android.service;

import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.android.R;
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
	
	private Binder binder = new ClientConnectorBinder();
	private NotificationManager notificationManager;
	private NXCSession session = null;
	private boolean connectionInProgress = false;
	
   /**
    * Class for clients to access.  Because we know this service always
    * runs in the same process as its clients, we don't need to deal with
    * IPC.
    */
   public class ClientConnectorBinder extends Binder 
   {
   	ClientConnectorService getService() 
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
	 * Reconnect to server if needed
	 */
	private void reconnect()
	{
		if ((session == null) && !connectionInProgress)
		{
			connectionInProgress = true;
			SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
			new ConnectTask(this).execute(sp.getString("connection.server", ""), sp.getString("connection.login", ""), sp.getString("connection.password", ""));
		}
	}
	
	/**
	 * Called by connect task after successful connection
	 * 
	 * @param session new session object
	 */
	public void onConnect(NXCSession session)
	{
		connectionInProgress = false;
		this.session = session;
		showNotification(NOTIFY_CONN_STATUS, getString(R.string.notify_connected, session.getServerAddress()));
		session.addListener(this);
	}
	
	/**
	 * Called by connect task or session notification listener after
	 * unsuccessful connection or disconnect
	 */
	public void onDisconnect()
	{
		if (session != null)
		{
			session.removeListener(this);
			showNotification(NOTIFY_CONN_STATUS, getString(R.string.notify_disconnected));
		}
		connectionInProgress = false;
		session = null;
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(SessionNotification n)
	{
	}
}
