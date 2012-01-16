/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;

import android.content.ComponentName;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.jjoe64.graphview.*; 
import com.jjoe64.graphview.GraphView.GraphViewSeries;
import com.jjoe64.graphview.GraphView.GraphViewData;
import com.jjoe64.graphview.GraphView.LegendAlign;

/**
 * @author Marco Incalcaterra
 *
 */
public class DrawGraph extends AbstractClientActivity
{
	private long nodeId = 0;
	private long dciId = 0;
	private long secsBack = 0;
	private GenericObject node = null;
	private GraphView graphView = null;
	private String title;

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		setContentView(R.layout.graphics);
		nodeId = getIntent().getLongExtra("nodeId", 0);
		dciId = getIntent().getLongExtra("dciId", 0);
		secsBack =  getIntent().getLongExtra("seconds", 0);
		title = getIntent().getStringExtra("title");
		graphView = new LineGraphView(this, "")
		{
			@Override
			protected String formatLabel(double value, boolean isValueX)
			{
				if (isValueX)
				{
					SimpleDateFormat s = new SimpleDateFormat("HH:mm:ss");
					return s.format(new Date((long)value));
				}
				else
					return super.formatLabel(value, isValueX); // let the y-value be normal-formatted
			}
		};
		graphView.setShowLegend(true);   
		graphView.setLegendAlign(LegendAlign.TOP);   
		graphView.setLegendWidth(240);  
		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.last_values_title);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
		if (nodeId > 0)
			node = service.findObjectById(nodeId);
		refreshGraph();
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
	 * Refresh node graph
	 */
	public void refreshGraph()
	{
		if (node != null)
		{
			TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
			title.setText(getString(R.string.graph_title) + ": " + node.getObjectName());
		}
		new LoadDataTask(nodeId).execute();
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		service.registerNodeBrowser(null);
		unbindService(this);
		super.onDestroy();
	}

	/**
	 * Internal task for loading DCI data
	 */
	private class LoadDataTask extends AsyncTask<Object, Void, DciData>
	{
		private long nodeId;
		
		protected LoadDataTask(long nodeId)
		{
			this.nodeId = nodeId;
		}
		
		@Override
		protected DciData doInBackground(Object... params)
		{
			try
			{
				final Date from = new Date(System.currentTimeMillis() - secsBack*1000);
				final Date to = new Date(System.currentTimeMillis());
				return service.getSession().getCollectedData(nodeId, dciId, from, to, 0);
			}
			catch(Exception e)
			{
				Log.d("nxclient/LastValues", "Exception while executing LoadDataTask.doInBackground", e);
				return null;
			}
		}

		@Override
		protected void onPostExecute(DciData result)
		{
			if (result != null)
			{
				DciDataRow[] dciData = result.getValues();
				GraphViewData[] graphData = new GraphViewData[dciData.length];
				for (int i=dciData.length-1, j=0; i>=0; i--, j++)	// dciData are reversed!
					graphData[j] = new GraphViewData(dciData[i].getTimestamp().getTime(), dciData[i].getValueAsDouble());  
				GraphViewSeries series = new GraphViewSeries(title, null, graphData);
				graphView.addSeries(series); // data
				LinearLayout layout = (LinearLayout) findViewById(R.id.graphics);   
				if (layout != null)
					layout.addView(graphView);
			}
		}
	}
}
