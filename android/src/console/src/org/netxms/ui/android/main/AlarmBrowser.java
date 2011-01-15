/**
 * 
 */
package org.netxms.ui.android.main;

import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.AlarmListAdapter;
import org.netxms.ui.android.service.ClientConnectorService;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.widget.ListView;

/**
 * Alarm browser
 *
 */
public class AlarmBrowser extends Activity implements ServiceConnection
{
	private ClientConnectorService service;
	private ListView listView;
	private AlarmListAdapter adapter;
	
	/* (non-Javadoc)
	 * @see android.app.Activity#onCreate(android.os.Bundle)
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.alarm_view);
		
		bindService(new Intent(this, ClientConnectorService.class), this, 0);

		listView = (ListView)findViewById(R.id.AlarmList);
		adapter = new AlarmListAdapter(this);
		listView.setAdapter(adapter);
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		service = ((ClientConnectorService.ClientConnectorBinder)binder).getService();
		refreshList();
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
	}

	/**
	 * Refresh alarm list
	 */
	private void refreshList()
	{
		adapter.setAlarms(service.getAlarms());
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		unbindService(this);
		super.onDestroy();
	}
}
