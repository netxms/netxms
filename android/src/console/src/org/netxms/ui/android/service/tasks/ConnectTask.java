/**
 * 
 */
package org.netxms.ui.android.service.tasks;

import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;
import android.util.Log;

/**
 * Connect task
 */
public class ConnectTask extends Thread
{
	private static final String TAG = "nxclient.ConnectTask";

	private ClientConnectorService service;
	private String server;
	private String login;
	private String password;
	private Map<Long, Alarm> alarms;

	/**
	 * @param service
	 */
	public ConnectTask(ClientConnectorService service)
	{
		this.service = service;
	}

	/**
	 * Execute task
	 * 
	 * @param server
	 * @param login
	 * @param password
	 */
	public void execute(String server, String login, String password)
	{
		this.server = server;
		this.login = login;
		this.password = password;
		start();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see java.lang.Thread#run()
	 */
	@Override
	public void run()
	{
		final NXCSession session = new NXCSession(server, login, password);
		service.setConnectionStatus("connecting...");
		try
		{
			Log.d(TAG, "calling session.connect()");
			session.connect();
			Log.d(TAG, "calling session.subscribe()");
			session.subscribe(NXCSession.CHANNEL_ALARMS | NXCSession.CHANNEL_OBJECTS);

			alarms = session.getAlarms(false);
			service.showToast(service.getString(R.string.notify_connected, session.getServerAddress()));
			service.setConnectionStatus("connected to " + session.getServerAddress());
			service.onConnect(session, alarms);
			service.loadTools();
		}
		catch(Exception e)
		{
			Log.d(TAG, "Exception on connect attempt", e);
			service.setConnectionStatus("Connect failed: " + e.getLocalizedMessage());
			service.onDisconnect();
		}
	}
}
