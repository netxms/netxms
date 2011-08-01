/**
 * 
 */
package org.netxms.ui.android.main.activities;

import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.LastValuesAdapter;
import org.netxms.ui.android.service.ClientConnectorService;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.widget.ListView;
import android.widget.TextView;

/**
 * Node browser
 * 
 */
public class LastValues extends Activity implements ServiceConnection
{
	private ClientConnectorService service;
	private ListView listView;
	private LastValuesAdapter adapter;
	private long nodeId = 0;
	private GenericObject node = null;

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onCreate(android.os.Bundle)
	 */
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.last_values);

		this.nodeId = this.getIntent().getLongExtra("objectId", 0);

		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.last_values_title);

		bindService(new Intent(this, ClientConnectorService.class), this, 0);
		// keeps current list of values as datasource for listview
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
		service = ((ClientConnectorService.ClientConnectorBinder)binder).getService();
		adapter.setService(service);
		if (this.nodeId > 0)
		{
			this.node = service.findObjectById(nodeId);
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
		adapter.setService(null);
	}

	/**
	 * Refresh node list
	 */
	public void refreshList()
	{
		if (this.node != null)
		{
			TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
			title.setText(getString(R.string.last_values_title) + ": " + node.getObjectName());
		}

		adapter.setValues(service.getLastValues(this.nodeId));
		adapter.notifyDataSetChanged();
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
}
