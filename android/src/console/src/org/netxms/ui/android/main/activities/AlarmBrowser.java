/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.ArrayList;

import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.AlarmListAdapter;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ListView;

/**
 * Alarm browser
 *
 */
public class AlarmBrowser extends AbstractClientActivity
{
	private ListView listView;
	private AlarmListAdapter adapter;
	private ArrayList<Integer> nodeIdList;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		setContentView(R.layout.alarm_view);
		nodeIdList = getIntent().getIntegerArrayListExtra("nodeIdList");
		
		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.alarms_title);

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
		super.onServiceConnected(name, binder);
		adapter.setService(service);
		service.registerAlarmBrowser(this);
		refreshList();
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
		super.onServiceDisconnected(name);
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
			case R.id.acknowledge:
				adapter.acknowledgeItem(al.getId());
				refreshList();
				return true;
			case R.id.terminate:
				adapter.terminateItem(al.getId());
				refreshList();
				return true;
			case R.id.viewlastvalues:
				GenericObject object = service.findObjectById(al.getSourceObjectId());
				if (object!=null)
				{
					Intent newIntent = new Intent(this, LastValues.class);
					newIntent.putExtra("objectId", object.getObjectId());
					startActivity(newIntent);
				}
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
		adapter.setAlarms(service.getAlarms(), nodeIdList);
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
