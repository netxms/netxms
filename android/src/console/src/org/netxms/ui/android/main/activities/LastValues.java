/**
 * 
 */
package org.netxms.ui.android.main.activities;

import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.LastValuesAdapter;
import android.content.ComponentName;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.widget.ListView;
import android.widget.TextView;

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
		unbindService(this);
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
}
