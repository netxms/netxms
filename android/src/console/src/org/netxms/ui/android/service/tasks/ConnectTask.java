/**
 * 
 */
package org.netxms.ui.android.service.tasks;

import org.netxms.client.NXCSession;
import org.netxms.ui.android.service.ClientConnectorService;
import android.os.AsyncTask;

/**
 * @author Victor
 *
 */
public class ConnectTask extends AsyncTask<String, String, NXCSession>
{
	private ClientConnectorService service;
	private Exception exception;
	
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
			service.showNotification(1, "Connected to server " + result.getServerAddress());
		else
			service.showNotification(1, exception.toString());
	}
}
