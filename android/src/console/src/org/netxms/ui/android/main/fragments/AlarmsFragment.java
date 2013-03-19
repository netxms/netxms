/**
 * 
 */
package org.netxms.ui.android.main.fragments;

import java.util.ArrayList;

import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.loaders.AlarmLoader;
import org.netxms.ui.android.main.adapters.AlarmListAdapter;

import android.content.Intent;
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
	private ArrayList<Integer> nodeIdList = null;
	private int lastPosition = -1;
	private boolean lastValuesMenuIsEnabled = true;
	private static final String SORT_KEY = "AlarmsSortBy";
	private final int[] menuSortIds = { R.id.sort_severity_asc, R.id.sort_severity_desc,
			R.id.sort_date_asc, R.id.sort_date_desc, R.id.sort_name_asc, R.id.sort_name_desc };

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
		lv.setOnItemClickListener(new AdapterView.OnItemClickListener()
		{
			@Override
			public void onItemClick(AdapterView<?> arg0, View arg1, int position, long arg3)
			{
				lastPosition = position;
			}
		});
		lv.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener()
		{
			@Override
			public boolean onItemLongClick(AdapterView<?> arg0, View arg1, int position, long arg3)
			{
				lastPosition = position;
				return false;
			}
		});
	}

	@Override
	public void refresh()
	{
		if (loader != null)
		{
			loader.setService(service);
			if (adapter != null)
				adapter.setService(service);
			loader.forceLoad();
		}
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
			adapter.setFilter(nodeIdList);
			adapter.setSortBy(PreferenceManager.getDefaultSharedPreferences(getActivity()).getInt(SORT_KEY, 0));
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
		if (!lastValuesMenuIsEnabled)
		{
			MenuItem item = menu.findItem(R.id.viewlastvalues);
			item.setVisible(false);
		}
		checkMenuSortItem(menu);
	}

	@Override
	public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
	{
		inflater.inflate(R.menu.alarm_actions, menu);
		if (!lastValuesMenuIsEnabled)
		{
			MenuItem item = menu.findItem(R.id.viewlastvalues);
			item.setVisible(false);
		}
		checkMenuSortItem(menu);
		super.onCreateOptionsMenu(menu, inflater);
	}

	@Override
	public void onPrepareOptionsMenu(Menu menu)
	{
		super.onPrepareOptionsMenu(menu);
		if (!lastValuesMenuIsEnabled)
		{
			MenuItem item = menu.findItem(R.id.viewlastvalues);
			item.setVisible(false);
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

	@Override
	public void setNodeId(long id)
	{
		super.setNodeId(id);
		if (nodeIdList == null)
			nodeIdList = new ArrayList<Integer>(0);
		else
			nodeIdList.clear();
		nodeIdList.add((int)nodeId);
	}

	public void setNodeIdList(ArrayList<Integer> list)
	{
		nodeIdList = list;
	}

	public void enableLastValuesMenu(boolean enable)
	{
		lastValuesMenuIsEnabled = enable;
	}

	private void checkMenuSortItem(Menu menu)
	{
		int sortBy = PreferenceManager.getDefaultSharedPreferences(getActivity()).getInt(SORT_KEY, 0);
		if (sortBy >= 0 && sortBy < menuSortIds.length)
		{
			MenuItem item = menu.findItem(menuSortIds[sortBy]);
			if (item != null)
				item.setChecked(true);
		}
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
			case R.id.viewlastvalues:
				if (service != null)
				{
					Alarm al = (Alarm)adapter.getItem(lastPosition);
					if (al != null)
					{
						GenericObject object = service.findObjectById(al.getSourceObjectId());
						if (object != null)
						{
							Intent newIntent = new Intent(getActivity(), NodeInfoFragment.class);
							newIntent.putExtra("objectId", object.getObjectId());
							newIntent.putExtra("tabId", NodeInfoFragment.TAB_LAST_VALUES_ID);
							startActivity(newIntent);
						}
					}
				}
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
		if (idList.size() == 0)	// Get last item highlighted
		{
			Alarm al = (Alarm)adapter.getItem(lastPosition);
			if (al != null)
				idList.add(al.getId());
		}
		return idList;
	}

	/**
	 * Slect/unselect all the checkboxes
	 * 
	 * @param select true to select all, false to clear all 
	 */
	private void selectAll(boolean select)
	{
		for (int i = 0; i < adapter.getCount(); i++)
			lv.setItemChecked(i, select);
	}
}
