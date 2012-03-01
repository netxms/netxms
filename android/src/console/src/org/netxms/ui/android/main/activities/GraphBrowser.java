package org.netxms.ui.android.main.activities;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.GraphAdapter;

import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.View;
import android.widget.ExpandableListView;
import android.widget.TextView;

/**
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 * Browser for predefined graphics
 * 
 */
public class GraphBrowser extends AbstractClientActivity
{
	private ExpandableListView listView;
	private GraphAdapter adapter;
	ProgressDialog dialog; 

	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		dialog = new ProgressDialog(this); 
		setContentView(R.layout.graph_view);
		
		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.graphs_title);

		// keeps current list of graphs as datasource for listview
		adapter = new GraphAdapter(this, new ArrayList<String>(), new ArrayList<ArrayList<GraphSettings>>());
		listView = (ExpandableListView)findViewById(R.id.GraphList);
	    listView.setOnChildClickListener(
    		new ExpandableListView.OnChildClickListener()
		    {
			    @Override
			    public boolean onChildClick(ExpandableListView parent, View v, int groupPosition, int childPosition, long id)
			    {
					GraphSettings gs = (GraphSettings)adapter.getChild(groupPosition,  childPosition);
					if (gs != null)
					{
						drawGraph(gs);
						return true;
					}
					return false;
			    }
		    });
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
		service.registerGraphBrowser(this);
		refreshList();
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
		super.onServiceDisconnected(name);
	}

	/**
	 * Draw graph based on stored settings
	 */
	public void drawGraph(GraphSettings gs)
	{
		if (gs != null)
		{
			Intent newIntent = new Intent(this, DrawGraph.class);
			ArrayList<Integer> nodeIdList = new ArrayList<Integer>();
			ArrayList<Integer> dciIdList = new ArrayList<Integer>();
			ArrayList<Integer> colorList = new ArrayList<Integer>();
			ArrayList<String> nameList = new ArrayList<String>();
			// Set values
			GraphItem[] items = gs.getItems();
			GraphItemStyle[] itemStyles = gs.getItemStyles();
			for (int i = 0; i < items.length; i++)
			{
				nodeIdList.add((int)items[i].getNodeId());
				dciIdList.add((int)items[i].getDciId());
				colorList.add(itemStyles[i].getColor());
				nameList.add(items[i].getDescription());
			}
			// Pass them to activity
			newIntent.putExtra("numGraphs", items.length);
			newIntent.putIntegerArrayListExtra("nodeIdList", nodeIdList);
			newIntent.putIntegerArrayListExtra("dciIdList", dciIdList);
			newIntent.putIntegerArrayListExtra("colorList", colorList);
			newIntent.putStringArrayListExtra("nameList", nameList);
			newIntent.putExtra("graphTitle", adapter.TrimGroup(gs.getName()));
			if (gs.getTimeFrameType() == GraphSettings.TIME_FRAME_FIXED)
			{
				newIntent.putExtra("timeFrom", gs.getTimeFrom().getTime());
				newIntent.putExtra("timeTo", gs.getTimeTo().getTime());
			}
			else	// Back from now
			{
				newIntent.putExtra("timeFrom", System.currentTimeMillis() - gs.getTimeRangeMillis());
				newIntent.putExtra("timeTo", System.currentTimeMillis());
			}
			startActivity(newIntent);
		}
	}
	
	/* (non-Javadoc)
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		service.registerGraphBrowser(null);
		unbindService(this);
		super.onDestroy();
	}

	/**
	 * Refresh graphs list reloading from server
	 */
	public void refreshList()
	{
		new LoadDataTask().execute();
	}
	
	/**
	 * Internal task for loading predefined graphs data
	 */
	private class LoadDataTask extends AsyncTask<Object, Void, List<GraphSettings>>
	{
		@Override
		protected void onPreExecute()
		{
			dialog.setMessage(getString(R.string.progress_gathering_data)); 
			dialog.setIndeterminate(true); 
			dialog.setCancelable(false);
			dialog.show();
		}
		@Override
		protected List<GraphSettings> doInBackground(Object... params)
		{
			List<GraphSettings> graphs = null;
			try {
				NXCSession session=service.getSession();
				if (session != null)
					graphs = session.getPredefinedGraphs();
			} catch (NXCException e) {
				Log.d("nxclient/GraphBrowser", "NXCException while executing LoadDataTask.doInBackground", e);
				e.printStackTrace();
			} catch (IOException e) {
				Log.d("nxclient/GraphBrowser", "IOException while executing LoadDataTask.doInBackground", e);
				e.printStackTrace();
			}
			return graphs;
		}
		@Override
		protected void onPostExecute(List<GraphSettings> result)
		{
			dialog.cancel();
			if (result != null)
			{
				adapter.setGraphs(result);
				adapter.notifyDataSetChanged();
			}
		}
	}
}
