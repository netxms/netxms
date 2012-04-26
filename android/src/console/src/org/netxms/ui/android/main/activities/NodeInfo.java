package org.netxms.ui.android.main.activities;

import java.util.ArrayList;

import org.netxms.client.datacollection.DciValue;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.Node;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.AlarmListAdapter;
import org.netxms.ui.android.main.adapters.LastValuesAdapter;
import org.netxms.ui.android.main.adapters.OverviewAdapter;
import org.netxms.ui.android.service.ClientConnectorService;

import android.app.TabActivity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuItem;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.TabHost.TabContentFactory;
import android.widget.TabHost.OnTabChangeListener;

/**
 * Draw graph activity
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */
public class NodeInfo extends TabActivity implements OnTabChangeListener, ServiceConnection 
{
	protected ClientConnectorService service;

	private TabHost tabHost;
	private ListView overviewListView;
	private ListView lastValuesListView;
	private ListView alarmsListView;
	private OverviewAdapter overviewAdapter; 
	private LastValuesAdapter lastValuesAdapter; 
	private AlarmListAdapter alarmsAdapter; 
	private String TAB_OVERVIEW = "";
	private String TAB_LAST_VALUES = "";
	private String TAB_ALARMS = "";
	private long nodeId = 0;
	private Node node = null;

	@Override
    public void onCreate(Bundle savedInstanceState)
	{
        super.onCreate(savedInstanceState);
        
		startService(new Intent(this, ClientConnectorService.class));
		bindService(new Intent(this, ClientConnectorService.class), this, 0);
      
        setContentView(R.layout.node_info);

		nodeId = getIntent().getLongExtra("objectId", 0);

		tabHost = getTabHost();
		tabHost.setOnTabChangedListener(this);

        TAB_OVERVIEW = getString(R.string.node_info_overview);
		overviewAdapter = new OverviewAdapter(this);
		overviewListView = (ListView) findViewById(R.id.overview);
		overviewListView.setAdapter(overviewAdapter);
	
		tabHost.addTab(tabHost.newTabSpec(TAB_OVERVIEW).setIndicator(TAB_OVERVIEW).setContent(new TabContentFactory()
		{
			public View createTabContent(String arg0)
			{
				return overviewListView;
			}
		}));

        TAB_LAST_VALUES = getString(R.string.node_info_last_values);
		lastValuesAdapter = new LastValuesAdapter(this);
		lastValuesListView = (ListView) findViewById(R.id.last_values);
		lastValuesListView.setAdapter(lastValuesAdapter);
		registerForContextMenu(lastValuesListView);

		tabHost.addTab(tabHost.newTabSpec(TAB_LAST_VALUES).setIndicator(TAB_LAST_VALUES).setContent(new TabContentFactory()
		{
			public View createTabContent(String arg0)
			{
				return lastValuesListView;
			}
		}));

        TAB_ALARMS = getString(R.string.node_info_alarms);
		alarmsAdapter = new AlarmListAdapter(this);
		alarmsListView = (ListView) findViewById(R.id.alarms);
		alarmsListView.setAdapter(alarmsAdapter);
		registerForContextMenu(alarmsListView);

		tabHost.addTab(tabHost.newTabSpec(TAB_ALARMS).setIndicator(TAB_ALARMS).setContent(new TabContentFactory()
		{
			public View createTabContent(String arg0)
			{
				return alarmsListView;
			}
		}));
		
		// NB Seems to be necessary to avoid overlap of other tabs!
		tabHost.setCurrentTab(2);
		tabHost.setCurrentTab(1);
		tabHost.setCurrentTab(0);
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
	

	@Override
	public void onTabChanged(String tabId)
	{
		if (tabId.equals(TAB_OVERVIEW))
			overviewAdapter.notifyDataSetChanged();
		else if (tabId.equals(TAB_LAST_VALUES))
			lastValuesAdapter.notifyDataSetChanged();
		else if (tabId.equals(TAB_ALARMS))
			alarmsAdapter.notifyDataSetChanged();
	}

	/* (non-Javadoc)
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
		title.setText(node.getObjectName());
		refreshOverview();
		refreshLastValues();
		refreshAlarms();
	}

	public void refreshOverview()
	{
		if (node != null)
		{
			overviewAdapter.setValues(node);			
			overviewAdapter.notifyDataSetChanged();
		}
	}

	public void refreshLastValues()
	{
		new LoadLastValuesTask(nodeId).execute();
	}

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
	
	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateContextMenu(android.view.ContextMenu, android.view.View, android.view.ContextMenu.ContextMenuInfo)
	 */
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getMenuInflater();
		if (v == lastValuesListView)
			inflater.inflate(R.menu.last_values_actions, menu);
		else if (v == alarmsListView)
		{
			inflater.inflate(R.menu.alarm_actions, menu);
			menu.getItem(2).setVisible(false); 
		}
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

		switch (item.getItemId())
		{
			// Last values
			case R.id.graph_half_hour:
				return drawGraph(1800, (DciValue) lastValuesAdapter.getItem(info.position));
			case R.id.graph_one_hour:
				return drawGraph(3600, (DciValue) lastValuesAdapter.getItem(info.position));
			case R.id.graph_two_hours:
				return drawGraph(7200, (DciValue) lastValuesAdapter.getItem(info.position));
			case R.id.graph_four_hours:
				return drawGraph(14400, (DciValue) lastValuesAdapter.getItem(info.position));
			case R.id.graph_one_day:
				return drawGraph(86400, (DciValue) lastValuesAdapter.getItem(info.position));
			case R.id.graph_one_week:
				return drawGraph(604800, (DciValue) lastValuesAdapter.getItem(info.position));
			// Alarms
			case R.id.acknowledge:
				alarmsAdapter.acknowledgeItem(((Alarm)alarmsAdapter.getItem(info.position)).getId());
				refreshAlarms();
				return true;
			case R.id.terminate:
				alarmsAdapter.terminateItem(((Alarm)alarmsAdapter.getItem(info.position)).getId());
				refreshAlarms();
				return true;
		}
		return super.onContextItemSelected(item);
	}

	private boolean drawGraph(long secsBack, DciValue val)
	{
		Intent newIntent = new Intent(this, DrawGraph.class);
		ArrayList<Integer> nodeIdList = new ArrayList<Integer>();
		ArrayList<Integer> dciIdList = new ArrayList<Integer>();
		ArrayList<Integer> colorList = new ArrayList<Integer>();
		ArrayList<Integer> lineWidthList = new ArrayList<Integer>();
		ArrayList<String> nameList = new ArrayList<String>();
		// Set values
		nodeIdList.add((int)nodeId);
		dciIdList.add((int)val.getId());
		colorList.add(Color.GREEN);
		lineWidthList.add(3);
		nameList.add(val.getDescription());
		// Pass them to activity
		newIntent.putExtra("numGraphs", 1);
		newIntent.putIntegerArrayListExtra("nodeIdList", nodeIdList);
		newIntent.putIntegerArrayListExtra("dciIdList", dciIdList);
		newIntent.putIntegerArrayListExtra("colorList", colorList);
		newIntent.putIntegerArrayListExtra("lineWidthList", lineWidthList);
		newIntent.putStringArrayListExtra("nameList", nameList);
		newIntent.putExtra("graphTitle", val.getDescription());
		newIntent.putExtra("timeFrom", System.currentTimeMillis() - secsBack * 1000);
		newIntent.putExtra("timeTo", System.currentTimeMillis());
		startActivity(newIntent);
		return true;
	}
	
	/**
	 * Internal task for loading DCI data
	 */
	private class LoadLastValuesTask extends AsyncTask<Object, Void, DciValue[]>
	{
		private long nodeId;
		
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
			catch(Exception e)
			{
				Log.d("nxclient/NodeInfo", "Exception while executing LoadLastValuesTask.doInBackground", e);
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
}
