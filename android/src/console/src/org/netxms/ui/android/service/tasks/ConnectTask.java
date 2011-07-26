/**
 * 
 */
package org.netxms.ui.android.service.tasks;

import java.util.Map;

import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;
import android.os.AsyncTask;

/**
 * Connect task
 *
 */
public class ConnectTask extends AsyncTask<String, String, NXCSession>
{
	private ClientConnectorService service;
	private Exception exception;
	private Map<Long, Alarm> alarms;
	
	private static final int NOTIFY_CONN_STATUS = 1;
	
	/**
	 * Create connect task.
	 * 
	 * @param service
	 */
	public ConnectTask(ClientConnectorService service)
	{
		this.service = service;
	}
	
	/* (non-Javadoc)
	 * @see android.os.AsyncTask#doInBackground(Params[])
	 */
	@Override
	protected NXCSession doInBackground(String... params)
	{
		NXCSession session = new NXCSession(params[0], params[1], params[2]);
		try
		{
			session.connect();
			session.subscribe(NXCSession.CHANNEL_ALARMS+NXCSession.CHANNEL_OBJECTS);
			alarms = session.getAlarms(false);
			service.showNotification(NOTIFY_CONN_STATUS, service.getString(R.string.notify_connected, session.getServerAddress()));
		}
		catch(Exception e)
		{
			session = null;
			exception = e;
		}
		return session;
	}

	/* (non-Javadoc)
	 * @see android.os.AsyncTask#onPostExecute(java.lang.Object)
	 */
	@Override
	protected void onPostExecute(NXCSession result)
	{
		if (result != null)
			service.onConnect(result, alarms);
		else
			service.onDisconnect();
	}
}
