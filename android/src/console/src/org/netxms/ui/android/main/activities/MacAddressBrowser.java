package org.netxms.ui.android.main.activities;

 import java.io.IOException;

import org.netxms.client.MacAddress;
import org.netxms.client.MacAddressFormatException;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.MacAddressListAdapter;
import org.netxms.ui.android.main.adapters.MacAddressListAdapter.MacAddressInfo;

import android.app.ProgressDialog;
import android.content.ComponentName;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

/**
 * MAC address search result browser
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 *
 */

public class MacAddressBrowser extends AbstractClientActivity
{
	private ListView listView;
	private MacAddressListAdapter adapter;
	ProgressDialog dialog; 

	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		dialog = new ProgressDialog(this); 
		setContentView(R.layout.macaddress_view);
		
		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(R.string.macaddress_title);

		// keeps current list of alarms as datasource for listview
		adapter = new MacAddressListAdapter(this);
		listView = (ListView)findViewById(R.id.MacAddressList);
		listView.setAdapter(adapter);
		
		final EditText edittext = (EditText) findViewById(R.id.MacAddressToSearch);
		edittext.setOnKeyListener(new OnKeyListener() 
		{    
			public boolean onKey(View v, int keyCode, KeyEvent event)
			{
				if ((event.getAction() == KeyEvent.ACTION_DOWN) && (keyCode == KeyEvent.KEYCODE_ENTER))	// If the event is a key-down event on the "enter" button
				{
					MacAddressInfo info = adapter.new MacAddressInfo();
					info.setMacAddress(edittext.getText().toString());
					new SearchMacAddressTask().execute(info);
					return true;        
				}
				return false;
			}
		});
	
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
		super.onServiceDisconnected(name);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		unbindService(this);
		super.onDestroy();
	}
	
	/**
	 * Internal task for loading info for MAC address search
	 */
	private class SearchMacAddressTask extends AsyncTask<Object, Void, MacAddressInfo>
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
		protected MacAddressInfo doInBackground(Object... params)
		{

			MacAddressInfo info = (MacAddressInfo)params[0];
			NXCSession session=service.getSession();
			if (info != null && session != null)
			{
				ConnectionPoint cp = null;
				Node host = null;
				Node bridge = null;
				Interface iface = null;
				info.setFound(false);
				info.setValid(false);
				try
				{
					cp = session.findConnectionPoint(MacAddress.parseMacAddress(info.getMacAddress()));
					info.setValid(true);
					if (cp != null)
					{
						session.syncMissingObjects(new long[] { cp.getLocalNodeId(), cp.getNodeId(), cp.getInterfaceId() }, false, NXCSession.OBJECT_SYNC_WAIT);
						host = (Node)session.findObjectById(cp.getLocalNodeId());
						bridge = (Node)session.findObjectById(cp.getNodeId());
						iface = (Interface)session.findObjectById(cp.getInterfaceId());
					}
				}
				catch (MacAddressFormatException e)
				{
					Log.d("nxclient/MacAddressBrowser", "MacAddressFormatException while executing searchMacAddress", e);
				}
				catch (NXCException e)
				{
					Log.d("nxclient/SearchMacAddressTask", "Exception while executing syncMissingObjects", e);
				}
				catch (IOException e)
				{
					Log.d("nxclient/SearchMacAddressTask", "Exception while executing syncMissingObjects", e);
				}

				if ((bridge != null) && (iface != null))
				{
					info.setFound(true);
					info.setNodeName(host != null ? host.getObjectName() : "");
					info.setMacAddress(cp.getLocalMacAddress().toString());
					info.setBridgeName(bridge.getObjectName());
					info.setInterfaceName(iface.getObjectName());
				}
			}
			return info;
		}
		@Override
		protected void onPostExecute(MacAddressInfo result)
		{
			dialog.cancel();
			if (result != null)
			{
				adapter.addMacAddressInfo(result);
				adapter.notifyDataSetChanged();
			}
		}
	}
}
