/**
 * 
 */
package org.netxms.ui.android.service.tasks;

import java.io.IOException;

import org.netxms.base.NXCommon;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;
import org.netxms.ui.android.service.ClientConnectorService.ConnectionStatus;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Log;

/**
 * Connect task
 */
public class ConnectTask extends Thread
{
	private static final String TAG = "nxclient.ConnectTask";

	private ClientConnectorService service;
	private String server;
	private Integer port;
	private String login;
	private String password;
	private Boolean encrypt;

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
	 * @param encrypt
	 */
	public void execute(String server, Integer port, String login, String password, Boolean encrypt)
	{
		this.server = server;
		this.port = port;
		this.login = login;
		this.password = password;
		this.encrypt = encrypt;
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
		if (service != null)
			if (isInternetOn())
			{
				service.setConnectionStatus(ConnectionStatus.CS_INPROGRESS, "");
				NXCSession session = service.getSession();
				if (session != null)
					try
					{
						session.checkConnection();
						service.setConnectionStatus(ConnectionStatus.CS_ALREADYCONNECTED, session.getServerAddress());
					}
					catch (NXCException e)
					{
						Log.d(TAG, "NXCException in checking connection", e);
						service.onError(e.getLocalizedMessage());
						session = null;
					}
					catch (IOException e)
					{
						Log.d(TAG, "IOException in checking connection", e);
						service.onError(e.getLocalizedMessage());
						session = null;
					}
				if (session == null)	// Already null or invalidated
				{	
					session = new NXCSession(server, port, login, password, encrypt);
					session.setConnClientInfo("nxmc-android/" + NXCommon.VERSION + "." + service.getString(R.string.build_number));
					try
					{
						Log.d(TAG, "calling session.connect()");
						session.connect();
						Log.d(TAG, "calling session.subscribe()");
						session.subscribe(NXCSession.CHANNEL_ALARMS | NXCSession.CHANNEL_OBJECTS);
						service.onConnect(session, session.getAlarms());
						service.loadTools();
					}
					catch(Exception e)
					{
						Log.d(TAG, "Exception on connect attempt", e);
						service.onError(e.getLocalizedMessage());
					}
				}
			}
			else
			{
				Log.d(TAG, "No internet connection");
				service.setConnectionStatus(ConnectionStatus.CS_NOCONNECTION, "");
			}
		else
			Log.d(TAG, "Service unavailable");
	}

	/**
	 * Check for internet connectivity 
	 */
	private boolean isInternetOn()
	{
		ConnectivityManager connec = (ConnectivityManager)service.getSystemService(Context.CONNECTIVITY_SERVICE);
		return connec.getNetworkInfo(ConnectivityManager.TYPE_WIFI).getState() == NetworkInfo.State.CONNECTED || 
		       connec.getNetworkInfo(ConnectivityManager.TYPE_MOBILE).getState() == NetworkInfo.State.CONNECTED;
	}

}
