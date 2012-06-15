/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.ArrayList;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.LastValuesAdapter;

import android.content.ComponentName;
import android.content.Intent;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;

/**
 * Last values view
 */
public class LastValues extends AbstractClientActivity
{
	private ListView listView;
	private LastValuesAdapter adapter;
	private long nodeId = 0;
	private GenericObject node = null;

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		setContentView(R.layout.last_values);

		nodeId = getIntent().getLongExtra("objectId", 0);

		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.last_values_title);

		adapter = new LastValuesAdapter(this);

		listView = (ListView)findViewById(R.id.ValueList);
		listView.setAdapter(adapter);
		registerForContextMenu(listView);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.
	 * ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
		if (nodeId > 0)
		{
			node = service.findObjectById(nodeId);
		}
		refreshList();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * android.content.ServiceConnection#onServiceDisconnected(android.content
	 * .ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
		super.onServiceDisconnected(name);
	}

	/**
	 * Refresh node list
	 */
	public void refreshList()
	{
		if (node != null)
		{
			TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
			title.setText(getString(R.string.last_values_title) + ": " + node.getObjectName());
		}
		new LoadDataTask(nodeId).execute();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		service.registerNodeBrowser(null);
		super.onDestroy();
	}
	
	/**
	 * Internal task for loading DCI data
	 */
	private class LoadDataTask extends AsyncTask<Object, Void, DciValue[]>
	{
		private long nodeId;
		
		protected LoadDataTask(long nodeId)
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
				Log.d("nxclient/LastValues", "Exception while executing LoadDataTask.doInBackground", e);
				return null;
			}
		}

		@Override
		protected void onPostExecute(DciValue[] result)
		{
			if ((result != null) && (LastValues.this.nodeId == nodeId))
			{
				adapter.setValues(result);			
				adapter.notifyDataSetChanged();
			}
		}
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateContextMenu(android.view.ContextMenu, android.view.View, android.view.ContextMenu.ContextMenuInfo)
	 */
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
	{
		android.view.MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.last_values_actions, menu);
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
		DciValue val = (DciValue) adapter.getItem(info.position);
		long secsBack=0;
		
		// process menu selection
		switch (item.getItemId())
		{
			case R.id.graph_half_hour:
				secsBack = 1800;
				break;
			case R.id.graph_one_hour:
				secsBack = 3600;
				break;
			case R.id.graph_two_hours:
				secsBack = 7200;
				break;
			case R.id.graph_four_hours:
				secsBack = 14400;
				break;
			case R.id.graph_one_day:
				secsBack = 86400;
				break;
			case R.id.graph_one_week:
				secsBack = 604800;
				break;
			default:
				return super.onContextItemSelected(item);
		}
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
}
