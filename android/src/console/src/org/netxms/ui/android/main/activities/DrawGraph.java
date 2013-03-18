/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.ArrayList;
import java.util.Date;

import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.views.ExtendedLineGraphView;

import android.app.ProgressDialog;
import android.content.ComponentName;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GraphView.GraphViewData;
import com.jjoe64.graphview.GraphView.LegendAlign;
import com.jjoe64.graphview.GraphViewSeries;
import com.jjoe64.graphview.GraphViewSeries.GraphViewSeriesStyle;

/**
 * Draw graph activity
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */
public class DrawGraph extends AbstractClientActivity
{
	private static final String TAG = "nxclient/DrawGraph";
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
		boolean showLegend = getIntent().getBooleanExtra("showLegend", true);
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
		graphView = new ExtendedLineGraphView(this, "");
		graphView.setShowLegend(showLegend);
		graphView.setLegendAlign(LegendAlign.TOP);
		graphView.setLegendWidth(240);
		graphView.setScalable(true);
		graphView.setScrollable(true);
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
			catch (Exception e)
			{
				Log.d(TAG, "Exception while executing LoadDataTask.doInBackground", e);
				dciData = null;
			}
			return dciData;
		}
		@Override
		protected void onPostExecute(DciData[] result)
		{
			int addedSeries = 0;
			if (result != null)
			{
				double start = 0;
				double end = 0;
				for (int i = 0; i < result.length; i++)
				{
					DciDataRow[] dciDataRow = result[i].getValues();
					if (dciDataRow.length > 0)
					{
						GraphViewData[] gwData = new GraphViewData[dciDataRow.length];
						// dciData are reversed!
						for (int j = dciDataRow.length - 1, k = 0; j >= 0; j--, k++)
							gwData[k] = new GraphViewData(dciDataRow[j].getTimestamp().getTime(), dciDataRow[j].getValueAsDouble());
						GraphViewSeries gwSeries = new GraphViewSeries(
								items[i].getDescription(),
								new GraphViewSeriesStyle(itemStyles[i].getColor(), itemStyles[i].getLineWidth()),
								gwData);
						graphView.addSeries(gwSeries);
						addedSeries++;
						start = dciDataRow[dciDataRow.length - 1].getTimestamp().getTime();
						end = dciDataRow[0].getTimestamp().getTime();
					}
				}
				if (addedSeries == 0) // Add an empty series when getting no data
				{
					GraphViewData[] gwData = new GraphViewData[] { new GraphViewData(0, 0) };
					GraphViewSeries gwSeries = new GraphViewSeries("", new GraphViewSeriesStyle(0xFFFFFF, 4), gwData);
					graphView.addSeries(gwSeries);
				}
				LinearLayout layout = (LinearLayout)findViewById(R.id.graphics);
				if (layout != null)
				{
					graphView.setViewPort(start, end - start + 1); // Start showing full graph
					layout.addView(graphView);
				}
			}
			dialog.cancel();
			if (addedSeries == 0)
				Toast.makeText(getApplicationContext(), getString(R.string.notify_no_data), Toast.LENGTH_SHORT).show();
		}
	}
}
