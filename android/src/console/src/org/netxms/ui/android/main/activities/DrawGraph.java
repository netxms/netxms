/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;

import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.ui.android.R;

import android.app.ProgressDialog;
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
import com.jjoe64.graphview.GraphView.GraphViewStyle;
import com.jjoe64.graphview.GraphView.LegendAlign;

/**
 * Draw graph activity
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */
public class DrawGraph extends AbstractClientActivity
{
	private GraphView graphView = null;
	private GraphItem[] items = null;
	private GraphItemStyle[] itemStyles = null;
	private int numGraphs = 0;
	private long timeFrom = 0;
	private long timeTo = 0;
	private String graphTitle = "";
	ProgressDialog dialog; 

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		dialog = new ProgressDialog(this); 
		setContentView(R.layout.graphics);
		numGraphs = getIntent().getIntExtra("numGraphs", 0);
		if (numGraphs > 0)
		{
			items = new GraphItem[numGraphs];
			itemStyles = new GraphItemStyle[numGraphs];
			ArrayList<Integer> nodeIdList = getIntent().getIntegerArrayListExtra("nodeIdList");
			ArrayList<Integer> dciIdList = getIntent().getIntegerArrayListExtra("dciIdList");
			ArrayList<Integer> colorList = getIntent().getIntegerArrayListExtra("colorList");
			ArrayList<Integer> lineWidthList = getIntent().getIntegerArrayListExtra("lineWidthList");
			ArrayList<String> nameList = getIntent().getStringArrayListExtra("nameList");
			for (int i = 0; i < numGraphs; i++)
			{
				items[i] = new GraphItem();
				items[i].setNodeId(nodeIdList.get(i));
				items[i].setDciId(dciIdList.get(i));
				items[i].setDescription(nameList.get(i));
				itemStyles[i] = new GraphItemStyle();
				itemStyles[i].setColor(colorList.get(i) | 0xFF000000);
				itemStyles[i].setLineWidth(lineWidthList.get(i));
			}
			timeFrom = getIntent().getLongExtra("timeFrom", 0);
			timeTo = getIntent().getLongExtra("timeTo", 0);
			graphTitle = getIntent().getStringExtra("graphTitle");
		}
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
				{
					if (value <= 1E3)
						return String.format("%.0f", value);
					else if (value <= 1E6)
						return String.format("%.1f K", value/1E3);
					else if (value < 1E9)
						return String.format("%.1f M", value/1E6);
					else if (value < 1E12)
						return String.format("%.1f G", value/1E9);
					else if (value < 1E15)
						return String.format("%.1f T", value/1E12);
					return super.formatLabel(value, isValueX);
				}
			}
		};
		graphView.setShowLegend(true);   
		graphView.setScalable(true);
		graphView.setScrollable(true);
		graphView.setLegendAlign(LegendAlign.TOP);   
		graphView.setLegendWidth(240);  
		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.graph_title);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
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
		if (graphTitle != null)
		{
			TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
			title.setText(graphTitle);
		}
		new LoadDataTask().execute();
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
	private class LoadDataTask extends AsyncTask<Object, Void, DciData[]>
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
		protected DciData[] doInBackground(Object... params)
		{
			DciData[] dciData = null;
			try
			{
				if (numGraphs > 0)
				{
					dciData = new DciData[numGraphs];
					for (int i = 0; i < numGraphs; i++)
					{
						dciData[i] = new DciData(0, 0);
						dciData[i] = service.getSession().getCollectedData(items[i].getNodeId(), items[i].getDciId(), new Date(timeFrom), new Date(timeTo), 0);
					}
				}
			}
			catch(Exception e)
			{
				Log.d("nxclient/DrawGraph", "Exception while executing LoadDataTask.doInBackground", e);
				dciData = null;
			}
			return dciData;
		}
		@Override
		protected void onPostExecute(DciData[] result)
		{
			if (result != null)
			{
				int addedSeries = 0;
				double start = 0;
				double end = 0;
				for (int i = 0; i < result.length; i++)
				{
					DciDataRow[] dciDataRow = result[i].getValues();
					if (dciDataRow.length > 0)
					{
						GraphViewData[] gwData = new GraphViewData[dciDataRow.length];
						for (int j = dciDataRow.length-1, k = 0; j >= 0; j--, k++)	// dciData are reversed!
							gwData[k] = new GraphViewData(dciDataRow[j].getTimestamp().getTime(), dciDataRow[j].getValueAsDouble());
						GraphViewSeries gwSeries = new GraphViewSeries(items[i].getDescription(), new GraphViewStyle(itemStyles[i].getColor(), itemStyles[i].getLineWidth()), gwData);
						graphView.addSeries(gwSeries);
						addedSeries++;
						start = dciDataRow[dciDataRow.length-1].getTimestamp().getTime();
						end = dciDataRow[0].getTimestamp().getTime();
					}
				}
				if (addedSeries == 0)	// Add an empty series when getting no data
				{
					GraphViewData[] gwData = new GraphViewData[] { new GraphViewData(0, 0) };  
					GraphViewSeries gwSeries = new GraphViewSeries("", new GraphViewStyle(0xFFFFFF, 4), gwData);
					graphView.addSeries(gwSeries);
				}
				LinearLayout layout = (LinearLayout)findViewById(R.id.graphics);   
				if (layout != null)
				{
					graphView.setViewPort(start, end-start+1);	// Start showing full graph
					layout.addView(graphView);
				}
			}
			dialog.cancel();
		}
	}
}
