package org.netxms.ui.android.main.activities;

import org.netxms.ui.android.NXApplication;
import org.netxms.ui.android.service.ClientConnectorService;

import android.app.TabActivity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;

/**
 * Abstract base class for all activities in the client. Implements
 * common functionality for connecting to service and handling common items
 * in options menu.
 */
public abstract class AbstractTabActivity extends TabActivity implements ServiceConnection
{
	protected ClientConnectorService service;

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreate(android.os.Bundle)
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		onCreateStep2(savedInstanceState);
		startService(new Intent(this, ClientConnectorService.class));
		bindService(new Intent(this, ClientConnectorService.class), this, 0);

		// the following is required if target API version is 14:
		//getActionBar().setHomeButtonEnabled(true);
	}

	/**
	 * Called by AbstractTabActivity.onCreate before service binding
	 * to allow inherited classes to do initialization before onServiceConnected call
	 * 
	 * @param savedInstanceState
	 */
	protected abstract void onCreateStep2(Bundle savedInstanceState);

	/* (non-Javadoc)
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		if (service != null)
			service.registerAlarmBrowser(null);
		unbindService(this);
		super.onDestroy();
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onResume()
	 */
	@Override
	protected void onResume()
	{
		super.onResume();
		NXApplication.activityResumed();
		if (service != null)
			service.reconnect(false);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onPause()
	 */
	@Override
	protected void onPause()
	{
		super.onStop();
		NXApplication.activityPaused();
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		service = ((ClientConnectorService.ClientConnectorBinder)binder).getService();
		if (service != null)
			service.reconnect(false);
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
	}
}
