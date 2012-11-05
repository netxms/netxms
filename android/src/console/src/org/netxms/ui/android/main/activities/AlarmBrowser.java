/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.lang.reflect.Method;
import java.util.ArrayList;

import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.AlarmListAdapter;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.SparseBooleanArray;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;

/**
 * Alarm browser
 * 
 * @author Victor Kirhenshtein
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 *
 */

public class AlarmBrowser extends AbstractClientActivity
{
	private static final String SORT_KEY = "AlarmsSortBy";
	private ListView listView;
	private AlarmListAdapter adapter;
	private ArrayList<Integer> nodeIdList;
	private Spinner sortBy;
	private static Method method_invalidateOptionsMenu;

	static
	{
		try
		{
			method_invalidateOptionsMenu = Activity.class.getMethod("invalidateOptionsMenu", new Class[0]);
		}
		catch (NoSuchMethodException e)
		{
			method_invalidateOptionsMenu = null;
		}
	}

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
		listView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
		listView.setAdapter(adapter);
		registerForContextMenu(listView);

		sortBy = (Spinner)findViewById(R.id.sortBy);
		sortBy.setOnItemSelectedListener(new OnItemSelectedListener()
		{
			@Override
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id)
			{
				adapter.setSortBy(position);
				refreshList();
			}
			@Override
			public void onNothingSelected(AdapterView<?> arg0)
			{
			}
		});
		sortBy.setSelection(PreferenceManager.getDefaultSharedPreferences(this).getInt(SORT_KEY, 0));
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
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.alarm_actions, menu);
		final SparseBooleanArray positions = listView.getCheckedItemPositions();
		if (positions != null)
			for (int i = 0; i < positions.size(); i++)
				if (positions.get(i))
				{
					hideMenuItem(menu, R.id.viewlastvalues);
					break;
				}
	}
	/**
	 * @param menu
	 * @param id
	 */
	private void hideMenuItem(ContextMenu menu, int id)
	{
		MenuItem item = menu.findItem(id);
		if (item != null)
			item.setVisible(false);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		if (method_invalidateOptionsMenu != null)
		{
			android.view.MenuInflater inflater = getMenuInflater();
			inflater.inflate(R.menu.alarm_actions, menu);
		}
		return super.onCreateOptionsMenu(menu);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onPrepareOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onPrepareOptionsMenu(Menu menu)
	{
		super.onPrepareOptionsMenu(menu);

		if (method_invalidateOptionsMenu == null)
		{
			menu.removeItem(R.id.selectall);
			menu.removeItem(R.id.unselectall);
			menu.add(Menu.NONE, R.id.selectall, Menu.NONE, getString(R.string.alarm_selectall));
			menu.add(Menu.NONE, R.id.unselectall, Menu.NONE, getString(R.string.alarm_unselectall));
		}
		return true;
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		int itemId = item.getItemId();
		if (itemId == R.id.selectall)
			selectAll(true);
		if (itemId == R.id.unselectall)
			selectAll(false);
		else if (itemId == R.id.viewlastvalues)
		{
			if (service != null)
			{
				AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
				if (info != null)
				{
					Alarm al = (Alarm)adapter.getItem(info.position);
					GenericObject object = service.findObjectById(al.getSourceObjectId());
					if (object != null)
					{
						Intent newIntent = new Intent(this, NodeInfo.class);
						newIntent.putExtra("objectId", object.getObjectId());
						newIntent.putExtra("tabId", NodeInfo.TAB_LAST_VALUES_ID);
						startActivity(newIntent);
					}
				}
				return true;
			}
		}
		else
		{
			ArrayList<Long> alarmIdList = getSelection(item);
			int count = alarmIdList.size();
			if (itemId == R.id.acknowledge)
			{
				for (int i = 0; i < count; i++)
					adapter.acknowledgeItem(alarmIdList.get(i), false);
				refreshList();
				return true;
			}
			else if (itemId == R.id.sticky_acknowledge)
			{
				for (int i = 0; i < count; i++)
					adapter.acknowledgeItem(alarmIdList.get(i), true);
				refreshList();
				return true;
			}
			else if (itemId == R.id.resolve)
			{
				for (int i = 0; i < count; i++)
					adapter.resolveItem(alarmIdList.get(i));
				refreshList();
				return true;
			}
			else if (itemId == R.id.terminate)
			{
				for (int i = 0; i < count; i++)
					adapter.terminateItem(alarmIdList.get(i));
				refreshList();
				return true;
			}
		}
		return super.onContextItemSelected(item);
	}

	private ArrayList<Long> getSelection(MenuItem item)
	{
		ArrayList<Long> idList = new ArrayList<Long>();
		final SparseBooleanArray positions = listView.getCheckedItemPositions();
		if (positions != null && positions.size() > 0)
			for (int i = 0; i < adapter.getCount(); i++)
				if (positions.get(i))
				{
					Alarm al = (Alarm)adapter.getItem(i);
					if (al != null)
						idList.add(al.getId());
				}
		if (idList.size() == 0)
		{
			AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
			Alarm al = (Alarm)adapter.getItem(info.position);
			if (al != null)
				idList.add(al.getId());
		}
		return idList;
	}

	private void selectAll(boolean select)
	{
		for (int i = 0; i < adapter.getCount(); i++)
			listView.setItemChecked(i, select);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		if (item.getItemId() == R.id.selectall)
			selectAll(true);
		else if (item.getItemId() == R.id.unselectall)
			selectAll(false);
		return super.onOptionsItemSelected(item);
	}

	/**
	 * Refresh alarm list
	 */
	public void refreshList()
	{
		if (service != null)
		{
			adapter.setAlarms(service.getAlarms(), nodeIdList);
			adapter.notifyDataSetChanged();
			final SparseBooleanArray positions = listView.getCheckedItemPositions();
			int count = positions != null ? Math.max(positions.size(), adapter.getCount()) : adapter.getCount();
			for (int i = 0; i < count; i++)
				listView.setItemChecked(i, false);
		}
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		if (service != null)
			service.registerAlarmBrowser(null);
		super.onDestroy();
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onStop()
	 */
	@Override
	protected void onStop()
	{
		super.onStop();

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		SharedPreferences.Editor editor = prefs.edit();
		editor.putInt(SORT_KEY, adapter.getSortBy());
		editor.commit();
	}
}
