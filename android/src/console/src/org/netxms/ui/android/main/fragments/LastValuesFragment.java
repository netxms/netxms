/**
 * 
 */
package org.netxms.ui.android.main.fragments;

import java.util.ArrayList;

import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.ui.android.R;
import org.netxms.ui.android.loaders.DciValueLoader;
import org.netxms.ui.android.main.activities.DrawBarChart;
import org.netxms.ui.android.main.activities.DrawGraph;
import org.netxms.ui.android.main.activities.DrawPieChart;
import org.netxms.ui.android.main.activities.TableLastValues;
import org.netxms.ui.android.main.adapters.LastValuesAdapter;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
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
 * Fragment for last values info
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class LastValuesFragment extends AbstractListFragment implements LoaderManager.LoaderCallbacks<DciValue[]>
{
	private static final Integer[] DEFAULT_COLORS = { 0x40699C, 0x9E413E, 0x7F9A48, 0x695185, 0x3C8DA3, 0xCC7B38, 0x4F81BD, 0xC0504D,
			0x9BBB59, 0x8064A2, 0x4BACC6, 0xF79646, 0xAABAD7, 0xD9AAA9, 0xC6D6AC, 0xBAB0C9 };
	private static final int MAX_COLORS = DEFAULT_COLORS.length;

	private LastValuesAdapter adapter = null;
	private DciValueLoader loader = null;
	private ListView lv = null;
	private long timeFrame = 0;
	private ArrayList<Long> items = null;

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState)
	{
		View v = inflater.inflate(R.layout.lastvalues_fragment, container, false);
		createProgress(v);
		return v;
	}

	@Override
	public void onActivityCreated(Bundle savedInstanceState)
	{
		super.onActivityCreated(savedInstanceState);
		adapter = new LastValuesAdapter(getActivity());
		setListAdapter(adapter);
		setListShown(false, true);
		loader = (DciValueLoader)getActivity().getSupportLoaderManager().initLoader(R.layout.lastvalues_fragment, null, this);
		if (loader != null)
		{
			loader.setObjId(nodeId);
			loader.setService(service);
		}
		lv = getListView();
		registerForContextMenu(lv);
	}

	@Override
	public void refresh()
	{
		if (loader != null)
		{
			loader.setObjId(nodeId);
			loader.setService(service);
			loader.forceLoad();
		}
	}

	@Override
	public Loader<DciValue[]> onCreateLoader(int arg0, Bundle arg1)
	{
		return new DciValueLoader(getActivity());
	}

	@Override
	public void onLoadFinished(Loader<DciValue[]> arg0, DciValue[] arg1)
	{
		setListShown(true, true);
		if (adapter != null)
		{
			adapter.setValues(arg1);
			adapter.notifyDataSetChanged();
		}
	}

	@Override
	public void onLoaderReset(Loader<DciValue[]> arg0)
	{
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getActivity().getMenuInflater();
		DciValue value = (DciValue)adapter.getItem(((AdapterView.AdapterContextMenuInfo)menuInfo).position);
		if (value != null)
		{
			switch (value.getDcObjectType())
			{
				case DataCollectionObject.DCO_TYPE_ITEM:
					inflater.inflate(R.menu.last_values_actions, menu);
					break;
				case DataCollectionObject.DCO_TYPE_TABLE:
					inflater.inflate(R.menu.last_values_table_actions, menu);
					break;
			}
		}
	}

	@Override
	public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
	{
		inflater.inflate(R.menu.last_values_actions, menu);
		super.onCreateOptionsMenu(menu, inflater);
	}

	@Override
	public void onPrepareOptionsMenu(Menu menu)
	{
		super.onPrepareOptionsMenu(menu);
	}

	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		if (!getUserVisibleHint())
			return false;
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

	/**
	 * Handles menu item selection for both Option and Context menus
	 * @param item	Menu item to handle
	 * @return true if menu has been properly handled
	 */
	private boolean handleItemSelection(MenuItem item)
	{
		ArrayList<Long> idList = getLastValuesSelection(item);
		switch (item.getItemId())
		{
			case R.id.table_last_value:
				return showTableLastValue(idList);
			case R.id.graph_ten_minutes:
				return drawGraph(10 * 60, idList);
			case R.id.graph_thirty_minutes:
				return drawGraph(30 * 60, idList);
			case R.id.graph_one_hour:
				return drawGraph(3600, idList);
			case R.id.graph_two_hours:
				return drawGraph(2 * 3600, idList);
			case R.id.graph_four_hours:
				return drawGraph(4 * 3600, idList);
			case R.id.graph_twelve_hours:
				return drawGraph(12 * 3600, idList);
			case R.id.graph_one_day:
				return drawGraph(86400, idList);
			case R.id.graph_five_days:
				return longExecutionDrawGraph(5 * 86400, idList);
			case R.id.graph_one_week:
				return longExecutionDrawGraph(7 * 86400, idList);
			case R.id.graph_one_month:
				return longExecutionDrawGraph(30 * 86400, idList);
			case R.id.graph_one_year:
				return longExecutionDrawGraph(365 * 86400, idList);
			case R.id.bar_chart:
				return drawComparisonChart(DrawBarChart.class);
			case R.id.pie_chart:
				return drawComparisonChart(DrawPieChart.class);
		}
		return false;
	}

	/**
	 * Get list of selected dci values
	 */
	private ArrayList<Long> getLastValuesSelection(MenuItem item)
	{
		ArrayList<Long> idList = new ArrayList<Long>();
		final SparseBooleanArray positions = lv.getCheckedItemPositions();
		if (positions != null && positions.size() > 0)
			for (int i = 0; i < adapter.getCount(); i++)
				if (positions.get(i))
					idList.add((long)i);
		if (idList.size() == 0)
		{
			AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
			idList.add((long)(info != null ? info.position : lv.getSelectedItemPosition()));
		}
		return idList;
	}

	/**
	 * Show last value for table DCI
	 * 
	 * @param idList	list of DCI to graph
	 * @return
	 */
	private boolean showTableLastValue(ArrayList<Long> idList)
	{
		if (idList.size() > 0)
		{
			DciValue value = (DciValue)adapter.getItem(idList.get(0).intValue());
			Intent newIntent = new Intent(getActivity(), TableLastValues.class);
			newIntent.putExtra("nodeId", (int)nodeId);
			newIntent.putExtra("dciId", (int)value.getId());
			newIntent.putExtra("description", value.getDescription());
			startActivity(newIntent);
		}
		return true;
	}

	/**
	 * Draw graph for the specified time slot after confirmation of long task execution
	 * 
	 * @param secsBack	seconds back since current time
	 * @param idList	list of DCI to graph
	 * @return	Always true
	 */
	private boolean longExecutionDrawGraph(long secsBack, ArrayList<Long> idList)
	{
		timeFrame = secsBack;
		items = idList;

		new AlertDialog.Builder(getActivity())
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.warning)
				.setMessage(R.string.long_time_execution)
				.setCancelable(true)
				.setPositiveButton(R.string.yes, new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface dialog, int which)
					{
						if (items != null)
							drawGraph(timeFrame, items);
					}
				})
				.setNegativeButton(R.string.no, null)
				.show();
		return true;
	}

	/**
	 * Draw graph for the specified time slot
	 * 
	 * @param secsBack	seconds back since current time
	 * @param idList	list of DCI to graph
	 * @return	Always true
	 */
	private boolean drawGraph(long secsBack, ArrayList<Long> idList)
	{
		if (idList.size() > 0)
		{
			ArrayList<Integer> nodeIdList = new ArrayList<Integer>();
			ArrayList<Integer> dciIdList = new ArrayList<Integer>();
			ArrayList<Integer> colorList = new ArrayList<Integer>();
			ArrayList<Integer> lineWidthList = new ArrayList<Integer>();
			ArrayList<String> nameList = new ArrayList<String>();

			// Set values
			int count = 0;
			for (int i = 0; i < idList.size() && count < MAX_COLORS; i++)
			{
				DciValue value = (DciValue)adapter.getItem(idList.get(i).intValue());
				if (value != null && value.getDcObjectType() == DataCollectionObject.DCO_TYPE_ITEM)
				{
					nodeIdList.add((int)nodeId);
					dciIdList.add((int)value.getId());
					colorList.add(DEFAULT_COLORS[count]);
					lineWidthList.add(3);
					nameList.add(value.getDescription());
					count++;
				}
			}

			// Pass them to activity
			if (count > 0)
			{
				Intent newIntent = new Intent(getActivity(), DrawGraph.class);
				if (count == 1)
					newIntent.putExtra("graphTitle", nameList.get(0));
				newIntent.putExtra("numGraphs", count);
				newIntent.putIntegerArrayListExtra("nodeIdList", nodeIdList);
				newIntent.putIntegerArrayListExtra("dciIdList", dciIdList);
				newIntent.putIntegerArrayListExtra("colorList", colorList);
				newIntent.putIntegerArrayListExtra("lineWidthList", lineWidthList);
				newIntent.putStringArrayListExtra("nameList", nameList);
				newIntent.putExtra("timeFrom", System.currentTimeMillis() - secsBack * 1000);
				newIntent.putExtra("timeTo", System.currentTimeMillis());
				startActivity(newIntent);
			}
		}
		return true;
	}

	/**
	 * Draw pie chart for selected DCIs
	 * 
	 * @return
	 */
	private boolean drawComparisonChart(Class<?> chartClass)
	{
		Intent newIntent = new Intent(getActivity(), chartClass);

		ArrayList<Integer> nodeIdList = new ArrayList<Integer>();
		ArrayList<Integer> dciIdList = new ArrayList<Integer>();
		ArrayList<Integer> colorList = new ArrayList<Integer>();
		ArrayList<String> nameList = new ArrayList<String>();

		// Set values
		int count = 0;
		final SparseBooleanArray positions = lv.getCheckedItemPositions();
		if (positions.size() > 0)
		{
			for (int i = 0; (i < adapter.getCount()) && (count < MAX_COLORS); i++)
			{
				if (!positions.get(i))
					continue;

				DciValue value = (DciValue)adapter.getItem(i);
				if (value.getDcObjectType() != DataCollectionObject.DCO_TYPE_ITEM)
					continue;

				nodeIdList.add((int)nodeId);
				dciIdList.add((int)value.getId());
				colorList.add(DEFAULT_COLORS[count]);
				nameList.add(value.getDescription());
				count++;
			}
		}

		// Pass them to activity
		if (count > 0)
		{
			newIntent.putExtra("numItems", count);
			newIntent.putIntegerArrayListExtra("nodeIdList", nodeIdList);
			newIntent.putIntegerArrayListExtra("dciIdList", dciIdList);
			newIntent.putIntegerArrayListExtra("colorList", colorList);
			newIntent.putStringArrayListExtra("nameList", nameList);
			startActivity(newIntent);
		}
		return true;
	}
}
