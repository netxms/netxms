/**
 * 
 */
package org.netxms.ui.android.service.tasks;

import org.netxms.client.NXCSession;
import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;

import android.os.AsyncTask;

/**
 * Async task for executing agent's action
 */
public class ExecActionTask extends AsyncTask<Object, Void, Exception>
{
	private ClientConnectorService service;

	/* (non-Javadoc)
	 * @see android.os.AsyncTask#doInBackground(Params[])
	 */
	@Override
	protected Exception doInBackground(Object... params)
	{
		service = (ClientConnectorService)params[3];
		try
		{
			if (((String)params[2]).equals("wakeup"))
				((NXCSession)params[0]).wakeupNode((Long)params[1]);
			else
				((NXCSession)params[0]).executeAction((Long)params[1], (String)params[2]);
		}
		catch (Exception e)
		{
			return e;
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see android.os.AsyncTask#onPostExecute(java.lang.Object)
	 */
	@Override
	protected void onPostExecute(Exception result)
	{
		if (result == null)
		{
			service.showToast(service.getString(R.string.notify_action_exec_success));
		}
		else
		{
			service.showToast(service.getString(R.string.notify_action_exec_fail, result.getLocalizedMessage()));
		}
	}
}
