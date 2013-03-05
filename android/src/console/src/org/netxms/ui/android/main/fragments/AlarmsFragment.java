/**
 * 
 */
package org.netxms.ui.android.main.fragments;

import java.util.ArrayList;

import org.netxms.client.events.Alarm;
import org.netxms.ui.android.R;
import org.netxms.ui.android.loaders.AlarmLoader;
import org.netxms.ui.android.main.adapters.AlarmListAdapter;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.util.SparseBooleanArray;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ListView;

/**
 * Fragment for alarms info
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class AlarmsFragment extends AbstractListFragment implements LoaderManager.LoaderCallbacks<Alarm[]>
{
	private AlarmListAdapter adapter = null;
	private AlarmLoader loader = null;
	private ListView lv = null;
	private static final String SORT_KEY = "NodeAlarmsSortBy";
	private final int[] menuSortIds = { R.id.sort_severity_asc, R.id.sort_severity_desc,
			R.id.sort_date_asc, R.id.sort_date_desc };

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState)
	{
		View v = inflater.inflate(R.layout.alarms_fragment, container, false);
		createProgress(v);
		return v;
	}

	@Override
	public void onActivityCreated(Bundle savedInstanceState)
	{
		super.onActivityCreated(savedInstanceState);
		adapter = new AlarmListAdapter(getActivity());
		setListAdapter(adapter);
		setListShown(false, true);
		adapter.setService(service);
		loader = (AlarmLoader)getActivity().getSupportLoaderManager().initLoader(R.layout.alarms_fragment, null, this);
		if (loader != null)
			loader.setService(service);
		lv = getListView();
		registerForContextMenu(lv);
	}

	@Override
	public void refresh()
	{
		if (loader != null)
			loader.setService(service);
		if (adapter != null)
			adapter.setService(service);
	}

	@Override
	public Loader<Alarm[]> onCreateLoader(int arg0, Bundle arg1)
	{
		return new AlarmLoader(getActivity());
	}

	@Override
	public void onLoadFinished(Loader<Alarm[]> arg0, Alarm[] arg1)
	{
		setListShown(true, true);
		if (adapter != null)
		{
			ArrayList<Integer> id = new ArrayList<Integer>(0);
			id.add((int)nodeId);
			adapter.setFilter(id);
			adapter.setValues(arg1);
			adapter.notifyDataSetChanged();
		}
	}

	@Override
	public void onLoaderReset(Loader<Alarm[]> arg0)
	{
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getActivity().getMenuInflater();
		inflater.inflate(R.menu.alarm_actions, menu);
		menu.removeItem(R.id.viewlastvalues);
		checkMenuSortItem(menu);
	}

	@Override
	public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
	{
		if (method_invalidateOptionsMenu != null)
		{
			inflater.inflate(R.menu.alarm_actions, menu);
			menu.removeItem(R.id.viewlastvalues);
		}
		checkMenuSortItem(menu);
		super.onCreateOptionsMenu(menu, inflater);
	}

	@Override
	public void onPrepareOptionsMenu(Menu menu)
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
	}

	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		if (handleItemSelection(item))
			return true;
		return super.onContextItemSelected(item);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		if (handleItemSelection(item))
			return true;
		return super.onOptionsItemSelected(item);
	}

	private void checkMenuSortItem(Menu menu)
	{
		int sortBy = PreferenceManager.getDefaultSharedPreferences(getActivity()).getInt(SORT_KEY, 0);
		MenuItem item = menu.findItem(menuSortIds[sortBy]);
		if (item != null)
			item.setChecked(true);
	}

	private void setNewSort(MenuItem item, int sortBy)
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getActivity());
		SharedPreferences.Editor editor = prefs.edit();
		editor.putInt(SORT_KEY, sortBy);
		editor.commit();
		item.setChecked(!item.isChecked());
		selectAll(false);
		adapter.setSortBy(sortBy);
		adapter.sort();
		adapter.notifyDataSetChanged();
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
			case R.id.acknowledge:
				adapter.doAction(AlarmListAdapter.ACKNOWLEDGE, getAlarmsSelection(item));
				selectAll(false);
				return true;
			case R.id.sticky_acknowledge:
				adapter.doAction(AlarmListAdapter.STICKY_ACKNOWLEDGE, getAlarmsSelection(item));
				selectAll(false);
				return true;
			case R.id.resolve:
				adapter.doAction(AlarmListAdapter.RESOLVE, getAlarmsSelection(item));
				selectAll(false);
				return true;
			case R.id.terminate:
				adapter.doAction(AlarmListAdapter.TERMINATE, getAlarmsSelection(item));
				selectAll(false);
				return true;
		}
		return false;
	}

	/**
	 * Get list of selected alarms
	 */
	private ArrayList<Long> getAlarmsSelection(MenuItem item)
	{
		ArrayList<Long> idList = new ArrayList<Long>();
		final SparseBooleanArray positions = lv.getCheckedItemPositions();
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
			Alarm al = (Alarm)adapter.getItem(info != null ? info.position : lv.getSelectedItemPosition());
			if (al != null)
				idList.add(al.getId());
		}
		return idList;
	}

	private void selectAll(boolean select)
	{
		for (int i = 0; i < adapter.getCount(); i++)
			lv.setItemChecked(i, select);
	}
}
