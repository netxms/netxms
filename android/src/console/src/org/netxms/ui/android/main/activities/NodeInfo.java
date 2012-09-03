package org.netxms.ui.android.main.activities;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.AlarmListAdapter;
import org.netxms.ui.android.main.adapters.InterfacesAdapter;
import org.netxms.ui.android.main.adapters.LastValuesAdapter;
import org.netxms.ui.android.main.adapters.OverviewAdapter;
import org.netxms.ui.android.service.ClientConnectorService;

import android.app.Activity;
import android.app.TabActivity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.Log;
import android.util.SparseBooleanArray;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ExpandableListView;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TabHost;
import android.widget.TabHost.OnTabChangeListener;
import android.widget.TabHost.TabContentFactory;
import android.widget.TextView;

/**
 * Node info activity
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class NodeInfo extends TabActivity implements OnTabChangeListener, ServiceConnection
{
	public static int TAB_OVERVIEW_ID = 0;
	public static int TAB_ALARMS_ID = 1;
	public static int TAB_LAST_VALUES_ID = 2;
	public static int TAB_INTERFACES_ID = 3;

	private static final String TAG = "nxclient/NodeInfo";
	private static final Integer[] DEFAULT_COLORS = { 0x40699C, 0x9E413E, 0x7F9A48, 0x695185, 0x3C8DA3, 0xCC7B38, 0x4F81BD, 0xC0504D,
			0x9BBB59, 0x8064A2, 0x4BACC6, 0xF79646, 0xAABAD7, 0xD9AAA9, 0xC6D6AC, 0xBAB0C9 };
	private static final String SORT_KEY = "NodeAlarmsSortBy";

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

	protected ClientConnectorService service;

	private TabHost tabHost;
	private ListView overviewListView;
	private ListView lastValuesListView;
	private ListView alarmsListView;
	private ExpandableListView interfacesListView;
	private OverviewAdapter overviewAdapter;
	private LastValuesAdapter lastValuesAdapter;
	private AlarmListAdapter alarmsAdapter;
	private InterfacesAdapter interfacesAdapter;
	private String TAB_OVERVIEW;
	private String TAB_LAST_VALUES;
	private String TAB_ALARMS;
	private String TAB_INTERFACES;
	private int tabId;
	private long nodeId;
	private Node node = null;
	private boolean recreateOptionsMenu = false;
	private Spinner sortBy;

	/* (non-Javadoc)
	 * @see android.app.ActivityGroup#onCreate(android.os.Bundle)
	 */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		startService(new Intent(this, ClientConnectorService.class));
		bindService(new Intent(this, ClientConnectorService.class), this, 0);

		setContentView(R.layout.node_info);

		nodeId = getIntent().getLongExtra("objectId", 0);
		tabId = getIntent().getIntExtra("tabId", TAB_OVERVIEW_ID);

		tabHost = getTabHost();
		tabHost.setOnTabChangedListener(this);

		TAB_OVERVIEW = getString(R.string.node_info_overview);
		overviewAdapter = new OverviewAdapter(this);
		overviewListView = (ListView)findViewById(R.id.overview);
		overviewListView.setAdapter(overviewAdapter);
		tabHost.addTab(tabHost.newTabSpec(TAB_OVERVIEW).setIndicator(
				TAB_OVERVIEW, getResources().getDrawable(R.drawable.ni_overview_tab)).setContent(
				new TabContentFactory()
				{
					@Override
					public View createTabContent(String arg0)
					{
						return overviewListView;
					}
				}));

		TAB_ALARMS = getString(R.string.node_info_alarms);
		alarmsAdapter = new AlarmListAdapter(this);
		alarmsListView = (ListView)findViewById(R.id.alarms);
		alarmsListView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
		alarmsListView.setAdapter(alarmsAdapter);
		registerForContextMenu(alarmsListView);
		tabHost.addTab(tabHost.newTabSpec(TAB_ALARMS).setIndicator(
				TAB_ALARMS, getResources().getDrawable(R.drawable.ni_alarms_tab)).setContent(
				new TabContentFactory()
				{
					@Override
					public View createTabContent(String arg0)
					{
						return alarmsListView;
					}
				}));

		sortBy = (Spinner)findViewById(R.id.sortBy);
		sortBy.setOnItemSelectedListener(new OnItemSelectedListener()
		{
			@Override
			public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id)
			{
				alarmsAdapter.setSortBy(position);
				refreshAlarms();
			}
			@Override
			public void onNothingSelected(AdapterView<?> arg0)
			{
			}
		});
		sortBy.setSelection(PreferenceManager.getDefaultSharedPreferences(this).getInt(SORT_KEY, 0));

		TAB_LAST_VALUES = getString(R.string.node_info_last_values);
		lastValuesAdapter = new LastValuesAdapter(this);
		lastValuesListView = (ListView)findViewById(R.id.last_values);
		lastValuesListView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
		lastValuesListView.setAdapter(lastValuesAdapter);
		registerForContextMenu(lastValuesListView);
		tabHost.addTab(tabHost.newTabSpec(TAB_LAST_VALUES).setIndicator(
				TAB_LAST_VALUES, getResources().getDrawable(R.drawable.ni_last_values_tab)).setContent(
				new TabContentFactory()
				{
					@Override
					public View createTabContent(String arg0)
					{
						return lastValuesListView;
					}
				}));

		TAB_INTERFACES = getString(R.string.node_info_interfaces);
		interfacesAdapter = new InterfacesAdapter(this, null, null);
		interfacesListView = (ExpandableListView)findViewById(R.id.interfaces);
//		interfacesListView.setOnChildClickListener(new ExpandableListView.OnChildClickListener() {
//			@Override
//			public boolean onChildClick(ExpandableListView parent, View v, int groupPosition, int childPosition, long id)
//			{
//				GraphSettings gs = (GraphSettings)interfacesAdapter.getChild(groupPosition, childPosition);
//				if (gs != null)
//				{
//					//drawGraph(gs);
//					return true;
//				}
//				return false;
//			}
//		});
		interfacesListView.setAdapter(interfacesAdapter);
		registerForContextMenu(interfacesListView);
		tabHost.addTab(tabHost.newTabSpec(TAB_INTERFACES).setIndicator(
				TAB_INTERFACES, getResources().getDrawable(R.drawable.ni_interfaces_tab)).setContent(
				new TabContentFactory()
				{
					@Override
					public View createTabContent(String arg0)
					{
						return interfacesListView;
					}
				}));

		// NB Seems to be necessary to avoid overlap of other tabs!
		tabHost.setCurrentTabByTag(TAB_INTERFACES);
		tabHost.setCurrentTabByTag(TAB_LAST_VALUES);
		tabHost.setCurrentTabByTag(TAB_ALARMS);
		tabHost.setCurrentTabByTag(TAB_OVERVIEW);

		tabHost.setCurrentTab(tabId);
	}
	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		service.registerAlarmBrowser(null);
		unbindService(this);
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
		editor.putInt(SORT_KEY, alarmsAdapter.getSortBy());
		editor.commit();
	}

	/* (non-Javadoc)
	 * @see android.widget.TabHost.OnTabChangeListener#onTabChanged(java.lang.String)
	 */
	@Override
	public void onTabChanged(String tabId)
	{
		if (sortBy != null)
			sortBy.setVisibility(tabId.equals(TAB_ALARMS) ? View.VISIBLE : View.INVISIBLE);
		if (tabId.equals(TAB_OVERVIEW))
			overviewAdapter.notifyDataSetChanged();
		else if (tabId.equals(TAB_LAST_VALUES))
			lastValuesAdapter.notifyDataSetChanged();
		else if (tabId.equals(TAB_ALARMS))
			alarmsAdapter.notifyDataSetChanged();
		else if (tabId.equals(TAB_INTERFACES))
			interfacesAdapter.notifyDataSetChanged();
		if (method_invalidateOptionsMenu != null)
		{
			try
			{
				method_invalidateOptionsMenu.invoke(this);
			}
			catch (Exception e)
			{
				Log.w(TAG, "onTabChanged", e);
			}
		}
		else
		{
			recreateOptionsMenu = true;
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onCreateContextMenu(android.view.ContextMenu, android.view.View,
	 * android.view.ContextMenu.ContextMenuInfo)
	 */
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getMenuInflater();
		if (v == lastValuesListView)
		{
			DciValue value = (DciValue)lastValuesAdapter.getItem(((AdapterView.AdapterContextMenuInfo)menuInfo).position);
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
		else if (v == alarmsListView)
		{
			inflater.inflate(R.menu.alarm_actions, menu);
			hideMenuItem(menu, R.id.viewlastvalues);
		}
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

			if (tabHost.getCurrentTab() == TAB_LAST_VALUES_ID)
			{
				inflater.inflate(R.menu.last_values_actions, menu);
			}
			else if (tabHost.getCurrentTab() == TAB_ALARMS_ID)
			{
				inflater.inflate(R.menu.alarm_actions, menu);
				hideMenuItem(menu, R.id.viewlastvalues);
			}
		}
		return super.onCreateOptionsMenu(menu);
	}

	private void hideMenuItem(Menu menu, int resId)
	{
		MenuItem item = menu.findItem(resId);
		if (item != null)
			item.setVisible(false);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onPrepareOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onPrepareOptionsMenu(Menu menu)
	{
		super.onPrepareOptionsMenu(menu);

		if (!recreateOptionsMenu)
			return true;

		if (tabHost.getCurrentTab() == TAB_LAST_VALUES_ID)
		{
			menu.add(Menu.NONE, R.id.graph_half_hour, Menu.NONE, getString(R.string.last_values_graph_half_hour));
			menu.add(Menu.NONE, R.id.graph_one_hour, Menu.NONE, getString(R.string.last_values_graph_one_hour));// .setIcon(R.drawable.ic_menu_line_chart);
			menu.add(Menu.NONE, R.id.graph_two_hours, Menu.NONE, getString(R.string.last_values_graph_two_hours));
			menu.add(Menu.NONE, R.id.graph_four_hours, Menu.NONE, getString(R.string.last_values_graph_four_hours));
			menu.add(Menu.NONE, R.id.graph_one_day, Menu.NONE, getString(R.string.last_values_graph_one_day));
			menu.add(Menu.NONE, R.id.graph_one_week, Menu.NONE, getString(R.string.last_values_graph_one_week));
			menu.add(Menu.NONE, R.id.bar_chart, Menu.NONE, getString(R.string.last_values_bar_chart));
			menu.add(Menu.NONE, R.id.pie_chart, Menu.NONE, getString(R.string.last_values_pie_chart));
		}
		else
		{
			menu.removeItem(R.id.graph_half_hour);
			menu.removeItem(R.id.graph_one_hour);
			menu.removeItem(R.id.graph_two_hours);
			menu.removeItem(R.id.graph_four_hours);
			menu.removeItem(R.id.graph_one_day);
			menu.removeItem(R.id.graph_one_week);
			menu.removeItem(R.id.bar_chart);
			menu.removeItem(R.id.pie_chart);
		}

		recreateOptionsMenu = false;
		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		service = ((ClientConnectorService.ClientConnectorBinder)binder).getService();
		node = (Node)service.findObjectById(nodeId);
		service.registerNodeInfo(this);
		alarmsAdapter.setService(service);
		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		if (node != null)
			title.setText(node.getObjectName());
		refreshOverview();
		refreshLastValues();
		refreshAlarms();
		refreshInterfaces();
	}

	/**
	 * 
	 */
	public void refreshOverview()
	{
		if (node != null)
		{
			overviewAdapter.setValues(node);
			overviewAdapter.notifyDataSetChanged();
		}
	}

	/**
	 * 
	 */
	public void refreshLastValues()
	{
		new LoadLastValuesTask(nodeId).execute();
	}

	/**
	 * 
	 */
	public void refreshAlarms()
	{
		if (node != null)
		{
			ArrayList<Integer> id = new ArrayList<Integer>(0);
			id.add((int)node.getObjectId());
			alarmsAdapter.setAlarms(service.getAlarms(), id);
			alarmsAdapter.notifyDataSetChanged();
		}
	}

	/**
	 * 
	 */
	public void refreshInterfaces()
	{
		if (node != null)
			new LoadChildrenTask(node.getObjectId(), GenericObject.OBJECT_INTERFACE).execute();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.content.ServiceConnection#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		// get selected item
		AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();

		switch (item.getItemId())
		{
		// Last values
			case R.id.graph_half_hour:
				return drawGraph(1800, info.position);
			case R.id.graph_one_hour:
				return drawGraph(3600, info.position);
			case R.id.graph_two_hours:
				return drawGraph(7200, info.position);
			case R.id.graph_four_hours:
				return drawGraph(14400, info.position);
			case R.id.graph_one_day:
				return drawGraph(86400, info.position);
			case R.id.graph_one_week:
				return drawGraph(604800, info.position);
			case R.id.bar_chart:
				return drawComparisonChart(DrawBarChart.class);
			case R.id.pie_chart:
				return drawComparisonChart(DrawPieChart.class);
			case R.id.table_last_value:
				return showTableLastValue(info.position);
				// Alarms
			case R.id.acknowledge:
				acknowledgeAlarms(info.position, false);
				return true;
			case R.id.sticky_acknowledge:
				acknowledgeAlarms(info.position, true);
				return true;
			case R.id.resolve:
				resolveAlarms(info.position);
				return true;
			case R.id.terminate:
				terminateAlarms(info.position);
				return true;
		}
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
		switch (item.getItemId())
		{
			case android.R.id.home:
				startActivity(new Intent(this, HomeScreen.class));
				return true;
			case R.id.settings:
				startActivity(new Intent(this, ConsolePreferences.class));
				return true;
				// Last values
			case R.id.graph_half_hour:
				return drawGraph(1800, lastValuesListView.getSelectedItemPosition());
			case R.id.graph_one_hour:
				return drawGraph(3600, lastValuesListView.getSelectedItemPosition());
			case R.id.graph_two_hours:
				return drawGraph(7200, lastValuesListView.getSelectedItemPosition());
			case R.id.graph_four_hours:
				return drawGraph(14400, lastValuesListView.getSelectedItemPosition());
			case R.id.graph_one_day:
				return drawGraph(86400, lastValuesListView.getSelectedItemPosition());
			case R.id.graph_one_week:
				return drawGraph(604800, lastValuesListView.getSelectedItemPosition());
			case R.id.bar_chart:
				return drawComparisonChart(DrawBarChart.class);
			case R.id.pie_chart:
				return drawComparisonChart(DrawPieChart.class);
			default:
				return super.onOptionsItemSelected(item);
		}
	}

	/**
	 * Show last value for table DCI
	 * 
	 * @param position
	 * @return
	 */
	private boolean showTableLastValue(int position)
	{
		DciValue value = (DciValue)lastValuesAdapter.getItem(position);
		Intent newIntent = new Intent(this, TableLastValues.class);
		newIntent.putExtra("nodeId", (int)nodeId);
		newIntent.putExtra("dciId", (int)value.getId());
		newIntent.putExtra("description", value.getDescription());
		startActivity(newIntent);
		return true;
	}

	/**
	 * @param secsBack
	 * @param val
	 * @return
	 */
	private boolean drawGraph(long secsBack, int position)
	{
		Intent newIntent = new Intent(this, DrawGraph.class);

		ArrayList<Integer> nodeIdList = new ArrayList<Integer>();
		ArrayList<Integer> dciIdList = new ArrayList<Integer>();
		ArrayList<Integer> colorList = new ArrayList<Integer>();
		ArrayList<Integer> lineWidthList = new ArrayList<Integer>();
		ArrayList<String> nameList = new ArrayList<String>();

		// Set values
		int count = 0;
		final SparseBooleanArray positions = lastValuesListView.getCheckedItemPositions();
		if (positions.size() > 0)
		{
			for (int i = 0; (i < lastValuesAdapter.getCount()) && (count < 16); i++)
			{
				if (!positions.get(i))
					continue;

				DciValue value = (DciValue)lastValuesAdapter.getItem(i);
				if (value.getDcObjectType() != DataCollectionObject.DCO_TYPE_ITEM)
					continue;

				nodeIdList.add((int)nodeId);
				dciIdList.add((int)value.getId());
				colorList.add(DEFAULT_COLORS[count]);
				lineWidthList.add(3);
				nameList.add(value.getDescription());
				count++;
			}
		}

		if ((count == 0) && (position != ListView.INVALID_POSITION))
		{
			DciValue value = (DciValue)lastValuesAdapter.getItem(position);
			nodeIdList.add((int)nodeId);
			dciIdList.add((int)value.getId());
			colorList.add(DEFAULT_COLORS[0]);
			lineWidthList.add(3);
			nameList.add(value.getDescription());
			newIntent.putExtra("graphTitle", value.getDescription());
			count++;
		}

		// Pass them to activity
		if (count > 0)
		{
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
		return true;
	}

	/**
	 * Draw pie chart for selected DCIs
	 * 
	 * @return
	 */
	private boolean drawComparisonChart(Class<?> chartClass)
	{
		Intent newIntent = new Intent(this, chartClass);

		ArrayList<Integer> nodeIdList = new ArrayList<Integer>();
		ArrayList<Integer> dciIdList = new ArrayList<Integer>();
		ArrayList<Integer> colorList = new ArrayList<Integer>();
		ArrayList<String> nameList = new ArrayList<String>();

		// Set values
		int count = 0;
		final SparseBooleanArray positions = lastValuesListView.getCheckedItemPositions();
		if (positions.size() > 0)
		{
			for (int i = 0; (i < lastValuesAdapter.getCount()) && (count < 16); i++)
			{
				if (!positions.get(i))
					continue;

				DciValue value = (DciValue)lastValuesAdapter.getItem(i);
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

	private ArrayList<Long> getAlarmIdList(int position)
	{
		int count = 0;
		ArrayList<Long> alarmIdList = new ArrayList<Long>();
		final SparseBooleanArray positions = alarmsListView.getCheckedItemPositions();
		if (positions != null && positions.size() > 0)
			for (int i = 0; i < alarmsAdapter.getCount(); i++)
				if (positions.get(i))
				{
					Alarm al = (Alarm)alarmsAdapter.getItem(i);
					if (al != null)
					{
						alarmIdList.add(al.getId());
						count++;
					}
				}
		if (count == 0 && (position != ListView.INVALID_POSITION))
		{
			Alarm al = (Alarm)alarmsAdapter.getItem(position);
			if (al != null)
				alarmIdList.add(al.getId());
		}
		return alarmIdList;
	}

	private void acknowledgeAlarms(int position, boolean sticky)
	{
		ArrayList<Long> alarmIdList = getAlarmIdList(position);
		for (int i = 0; i < alarmIdList.size(); i++)
			alarmsAdapter.acknowledgeItem(alarmIdList.get(i), sticky);
		if (alarmIdList.size() != 0)
			refreshAlarms();
	}

	private void resolveAlarms(int position)
	{
		ArrayList<Long> alarmIdList = getAlarmIdList(position);
		for (int i = 0; i < alarmIdList.size(); i++)
			alarmsAdapter.resolveItem(alarmIdList.get(i));
		if (alarmIdList.size() != 0)
			refreshAlarms();
	}

	private void terminateAlarms(int position)
	{
		ArrayList<Long> alarmIdList = getAlarmIdList(position);
		for (int i = 0; i < alarmIdList.size(); i++)
			alarmsAdapter.terminateItem(alarmIdList.get(i));
		if (alarmIdList.size() != 0)
			refreshAlarms();
	}

	/**
	 * Internal task for loading DCI data
	 */
	private class LoadLastValuesTask extends AsyncTask<Object, Void, DciValue[]>
	{
		private final long nodeId;

		protected LoadLastValuesTask(long nodeId)
		{
			this.nodeId = nodeId;
		}

		@Override
		protected DciValue[] doInBackground(Object... params)
		{
			try
			{
				return service.getSession().getLastValues(nodeId);
			}
			catch (Exception e)
			{
				Log.d(TAG, "Exception while executing LoadLastValuesTask.doInBackground", e);
				return null;
			}
		}

		@Override
		protected void onPostExecute(DciValue[] result)
		{
			if ((result != null) && (NodeInfo.this.nodeId == nodeId))
			{
				lastValuesAdapter.setValues(result);
				lastValuesAdapter.notifyDataSetChanged();
			}
		}
	}

	/**
	 * Internal task for loading object children data
	 */
	private class LoadChildrenTask extends AsyncTask<Object, Void, Set<GenericObject>>
	{
		private final long nodeId;
		private final int classFilter;

		protected LoadChildrenTask(long nodeId, int classFilter)
		{
			this.nodeId = nodeId;
			this.classFilter = classFilter;
		}

		@Override
		protected Set<GenericObject> doInBackground(Object... params)
		{
			try
			{
				GenericObject obj = service.getSession().findObjectById(nodeId);
				if (obj != null)
				{
					service.getSession().syncMissingObjects(obj.getChildIdList(), false, NXCSession.OBJECT_SYNC_WAIT);
					return obj.getAllChilds(classFilter);
				}
			}
			catch (Exception e)
			{
				Log.d(TAG, "Exception while executing LoadChildrenTask.doInBackground", e);
			}
			return null;
		}

		@Override
		protected void onPostExecute(Set<GenericObject> result)
		{
			if (result != null) 
			{
				switch (classFilter)
				{
					case GenericObject.OBJECT_INTERFACE:
						List<Interface> interfaces = new ArrayList<Interface>();
						for (GenericObject i : result)
							interfaces.add((Interface)i);
						interfacesAdapter.setValues(interfaces);
						interfacesAdapter.notifyDataSetChanged();
						break;
				}
			}
		}
	}
}
