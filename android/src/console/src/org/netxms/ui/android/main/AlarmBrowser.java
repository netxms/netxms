/**
 * 
 */
package org.netxms.ui.android.main;

import org.netxms.client.events.Alarm;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.AlarmListAdapter;
import org.netxms.ui.android.service.ClientConnectorService;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
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

		// keeps current list of alarms as datasource for listview
		adapter = new AlarmListAdapter(this);

		listView = (ListView)findViewById(R.id.AlarmList);
		listView.setAdapter(adapter);
		registerForContextMenu(listView);
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		service = ((ClientConnectorService.ClientConnectorBinder)binder).getService();
		// make sure adapter can read required additional data
		adapter.setService(service);
		// remember this activity in service, so that service can send updates
		service.registerAlarmBrowser(this);
		refreshList();
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
		adapter.setService(null);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateContextMenu(android.view.ContextMenu, android.view.View, android.view.ContextMenu.ContextMenuInfo)
	 */
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v,ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.alarm_actions, menu);
	}
	
	/* (non-Javadoc)
	 * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		// get selected item
		AdapterView.AdapterContextMenuInfo info =
				(AdapterContextMenuInfo) item.getMenuInfo();
		Alarm al = (Alarm) adapter.getItem(info.position);

		// process menu selection
		switch (item.getItemId())
		{
			case R.id.terminate:
				adapter.terminateItem(al.getId());
				refreshList();
				return true;
			default:
				return super.onContextItemSelected(item);
		}
	}
	
	/**
	 * Refresh alarm list
	 */
	public void refreshList()
	{
		adapter.setAlarms(service.getAlarms());
		adapter.notifyDataSetChanged();
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		service.registerAlarmBrowser(null);
		unbindService(this);
		super.onDestroy();
	}
}
