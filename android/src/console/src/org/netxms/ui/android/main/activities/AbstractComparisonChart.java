package org.netxms.ui.android.main.activities;

import java.util.ArrayList;
import org.achartengine.GraphicalView;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.DciDataRow;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.android.R;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.TextView;

public abstract class AbstractComparisonChart extends AbstractClientActivity
{
	protected ArrayList<Integer> colorList = null;
	protected GraphItem[] items = null;
	protected double[] values = null;
	protected String graphTitle = "";

	private GraphicalView graphView = null;
	private ProgressDialog dialog;

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		dialog = new ProgressDialog(this); 
		setContentView(R.layout.graphics);
		int numItems = getIntent().getIntExtra("numItems", 0);
		if (numItems > 0)
		{
			items = new GraphItem[numItems];
			values = new double[numItems];
			ArrayList<Integer> nodeIdList = getIntent().getIntegerArrayListExtra("nodeIdList");
			ArrayList<Integer> dciIdList = getIntent().getIntegerArrayListExtra("dciIdList");
			ArrayList<String> nameList = getIntent().getStringArrayListExtra("nameList");
			colorList = getIntent().getIntegerArrayListExtra("colorList");
			for (int i = 0; i < numItems; i++)
			{
				items[i] = new GraphItem();
				items[i].setNodeId(nodeIdList.get(i));
				items[i].setDciId(dciIdList.get(i));
				items[i].setDescription(nameList.get(i));
			}
			graphTitle = getIntent().getStringExtra("graphTitle");
		}
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

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onServiceDisconnected(android.content.ComponentName)
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
	 * Convert to Android color format (swap RGB and add alpha)
	 */
	protected int toAndroidColor(int color)
	{
		return 0xFF000000 | ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);	// Alpha | R | G | B
	}
	
	/**
	 * @return
	 */
	protected abstract GraphicalView buildGraphView();
	
	/**
	 * Internal task for loading DCI data
	 */
	class LoadDataTask extends AsyncTask<Object, Void, DciData[]>
	{
		/* (non-Javadoc)
		 * @see android.os.AsyncTask#onPreExecute()
		 */
		@Override
		protected void onPreExecute()
		{
			dialog.setMessage(getString(R.string.progress_gathering_data)); 
			dialog.setIndeterminate(true); 
			dialog.setCancelable(false);
			dialog.show();
		}
		
		/* (non-Javadoc)
		 * @see android.os.AsyncTask#doInBackground(Params[])
		 */
		@Override
		protected DciData[] doInBackground(Object... params)
		{
			DciData[] dciData = null;
			try
			{
				if (items.length > 0)
				{
					dciData = new DciData[items.length];
					for (int i = 0; i < dciData.length; i++)
					{
						dciData[i] = new DciData(0, 0);
						dciData[i] = service.getSession().getCollectedData(items[i].getNodeId(), items[i].getDciId(), null, null, 1);
					}
				}
			}
			catch(Exception e)
			{
				Log.d("nxclient/AbstractComparisonChart", "Exception while executing LoadDataTask.doInBackground", e);
				dciData = null;
			}
			return dciData;
		}

		/* (non-Javadoc)
		 * @see android.os.AsyncTask#onPostExecute(java.lang.Object)
		 */
		@Override
		protected void onPostExecute(DciData[] result)
		{
			if (result != null)
			{
				for (int i = 0; (i < result.length) && (i < values.length); i++)
				{
					DciDataRow value = result[i].getLastValue();
					values[i] = (value != null) ? value.getValueAsDouble() : 0.0;
				}

				LinearLayout layout = (LinearLayout)findViewById(R.id.graphics);   
				if (layout != null)
				{
					if (graphView != null)
						layout.removeView(graphView);
					graphView = buildGraphView();
					layout.addView(graphView);
				}
			}
			dialog.cancel();
		}
	}
}
