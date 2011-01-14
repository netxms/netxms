/**
 * 
 */
package org.netxms.ui.android.service;

import org.netxms.ui.android.R;
import org.netxms.ui.android.main.HomeScreen;
import org.netxms.ui.android.service.tasks.ConnectTask;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.IBinder;
import android.preference.PreferenceManager;

/**
 * Background communication service for NetXMS client.
 *
 */
public class ClientConnectorService extends Service
{
	private Binder binder = new ClientConnectorBinder();
	private NotificationManager notificationManager;
	
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
		
		reconnect();
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
	
	private void reconnect()
	{
		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
		new ConnectTask(this).execute(sp.getString("connection.server", ""), sp.getString("connection.login", ""), sp.getString("connection.password", ""));
	}
}
