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
import org.netxms.ui.android.main.fragments.NodeInfoFragment;

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
import android.widget.ListView;
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
	private final int[] menuSortIds = { R.id.sort_severity_asc, R.id.sort_severity_desc,
			R.id.sort_date_asc, R.id.sort_date_desc, R.id.sort_name_asc, R.id.sort_name_desc };
	private ListView listView;
	private AlarmListAdapter adapter;
	private ArrayList<Integer> nodeIdList;
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
		checkMenuSortItem(menu);
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

	private void checkMenuSortItem(Menu menu)
	{
		int sortBy = PreferenceManager.getDefaultSharedPreferences(this).getInt(SORT_KEY, 0);
		MenuItem item = menu.findItem(menuSortIds[sortBy]);
		if (item != null)
			item.setChecked(true);
	}

	private void setNewSort(MenuItem item, int sortBy)
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		SharedPreferences.Editor editor = prefs.edit();
		editor.putInt(SORT_KEY, sortBy);
		editor.commit();
		item.setChecked(!item.isChecked());
		selectAll(false);
		adapter.setSortBy(sortBy);
		adapter.sort();
		adapter.notifyDataSetChanged();
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
		checkMenuSortItem(menu);
		return super.onCreateOptionsMenu(menu);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onPrepareOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onPrepareOptionsMenu(Menu menu)
	{
		super.onPrepareOptionsMenu(menu);

		menu.removeItem(R.id.viewlastvalues);
		if (method_invalidateOptionsMenu == null)
		{
			menu.removeItem(R.id.selectall);
			menu.removeItem(R.id.unselectall);
			menu.add(Menu.NONE, R.id.selectall, Menu.NONE, getString(R.string.alarm_selectall));
			menu.add(Menu.NONE, R.id.unselectall, Menu.NONE, getString(R.string.alarm_unselectall));
		}
		checkMenuSortItem(menu);
		return true;
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		if (handleItemSelection(item))
			return true;
		return super.onContextItemSelected(item);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		if (handleItemSelection(item))
			return true;
		return super.onOptionsItemSelected(item);
	}

	/**
	 * Handles menu item selection for both Option and Context menus
	 * @param item	Menu item to handle
	 * @return true if menu has been properly handled
	 */
	private boolean handleItemSelection(MenuItem item)
	{
		switch (item.getItemId())
		{
			case R.id.sort_severity_asc:
				setNewSort(item, AlarmListAdapter.SORT_SEVERITY_ASC);
				return true;
			case R.id.sort_severity_desc:
				setNewSort(item, AlarmListAdapter.SORT_SEVERITY_DESC);
				return true;
			case R.id.sort_date_asc:
				setNewSort(item, AlarmListAdapter.SORT_DATE_ASC);
				return true;
			case R.id.sort_date_desc:
				setNewSort(item, AlarmListAdapter.SORT_DATE_DESC);
				return true;
			case R.id.sort_name_asc:
				setNewSort(item, AlarmListAdapter.SORT_NAME_ASC);
				return true;
			case R.id.sort_name_desc:
				setNewSort(item, AlarmListAdapter.SORT_NAME_DESC);
				return true;
			case R.id.selectall:
				selectAll(true);
				return true;
			case R.id.unselectall:
				selectAll(false);
				return true;
			case R.id.viewlastvalues:
				if (service != null)
				{
					AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
					if (info != null)
					{
						Alarm al = (Alarm)adapter.getItem(info.position);
						if (al != null)
						{
							GenericObject object = service.findObjectById(al.getSourceObjectId());
							if (object != null)
							{
//								Intent newIntent = new Intent(this, NodeInfo.class);
								Intent newIntent = new Intent(this, NodeInfoFragment.class);
								newIntent.putExtra("objectId", object.getObjectId());
//								newIntent.putExtra("tabId", NodeInfo.TAB_LAST_VALUES_ID);
								newIntent.putExtra("tabId", NodeInfoFragment.TAB_LAST_VALUES_ID);
								startActivity(newIntent);
							}
						}
					}
				}
				return true;
			case R.id.acknowledge:
				adapter.doAction(AlarmListAdapter.ACKNOWLEDGE, getSelection(item));
				return true;
			case R.id.sticky_acknowledge:
				adapter.doAction(AlarmListAdapter.STICKY_ACKNOWLEDGE, getSelection(item));
				selectAll(false);
				return true;
			case R.id.resolve:
				adapter.doAction(AlarmListAdapter.RESOLVE, getSelection(item));
				selectAll(false);
				return true;
			case R.id.terminate:
				adapter.doAction(AlarmListAdapter.TERMINATE, getSelection(item));
				selectAll(false);
				return true;
		}
		return false;
	}

	/**
	 * Get list of selected items
	 */
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
			Alarm al = (Alarm)adapter.getItem(info != null ? info.position : listView.getSelectedItemPosition());
			if (al != null)
				idList.add(al.getId());
		}
		return idList;
	}

	/**
	 * Select/unselect all items
	 * @param select true to select, false to unselect
	 */
	private void selectAll(boolean select)
	{
		for (int i = 0; i < adapter.getCount(); i++)
			listView.setItemChecked(i, select);
	}

	/**
	 * Refresh alarm list
	 */
	public void refreshList()
	{
		if (service != null)
		{
			adapter.setFilter(nodeIdList);
			adapter.setValues(service.getAlarms());
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
}
