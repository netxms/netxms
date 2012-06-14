package org.netxms.ui.android.main.activities;

import org.netxms.base.NXCommon;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.ActivityListAdapter;
import org.netxms.ui.android.service.ClientConnectorService.ConnectionStatus;
import android.content.ComponentName;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Home screen activity
 * 
 * @author Victor Kirhenshtein
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class HomeScreen extends AbstractClientActivity implements OnItemClickListener
{
	public static final int ACTIVITY_ALARMS = 1;
	public static final int ACTIVITY_DASHBOARDS = 2;
	public static final int ACTIVITY_NODES = 3;
	public static final int ACTIVITY_GRAPHS = 4;
	public static final int ACTIVITY_MACADDRESS = 5;

	TextView statusText;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	public void onCreateStep2(Bundle savedInstanceState)
	{
		setContentView(R.layout.homescreen);

		GridView gridview = (GridView)findViewById(R.id.ActivityList);
		if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE)
			gridview.setNumColumns(4);
		else
			gridview.setNumColumns(2);
		gridview.setAdapter(new ActivityListAdapter(this));

		gridview.setOnItemClickListener(this);

		statusText = (TextView)findViewById(R.id.ScreenTitleSecondary);

		TextView buildName = (TextView)findViewById(R.id.MainScreenVersion);
		buildName.setText(getString(R.string.version) + " " + NXCommon.VERSION + " (" + getString(R.string.build_number) + ")");
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onServiceConnected(android.content.ComponentName,
	 * android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
		service.registerHomeScreen(this);
		setStatusText(service.getConnectionStatusText(), service.getConnectionStatusColor());
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onCreateOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		super.onCreateOptionsMenu(menu);
		menu.add(Menu.NONE, R.string.exit, Menu.NONE, getString(R.string.exit));
		menu.findItem(R.string.exit).setIcon(android.R.drawable.ic_menu_close_clear_cancel);
		menu.removeItem(android.R.id.home);
		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch(item.getItemId())
		{
			case R.string.exit:
				if (service != null)
					service.shutdown();
				moveTaskToBack(true);
				System.exit(0);
				return true;
			default:
				return super.onOptionsItemSelected(item);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.widget.AdapterView.OnItemClickListener#onItemClick(android.widget.AdapterView, android.view.View, int, long)
	 */
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id)
	{
		// Avoid starting activity if no connection
		if (service != null
				&& (service.getConnectionStatus() == ConnectionStatus.CS_CONNECTED || service.getConnectionStatus() == ConnectionStatus.CS_ALREADYCONNECTED))
			switch((int)id)
			{
				case ACTIVITY_ALARMS:
					startActivity(new Intent(this, AlarmBrowser.class));
					break;
				case ACTIVITY_NODES:
					startActivity(new Intent(this, NodeBrowser.class));
					break;
				case ACTIVITY_GRAPHS:
					startActivity(new Intent(this, GraphBrowser.class));
					break;
				case ACTIVITY_MACADDRESS:
					startActivity(new Intent(this, ConnectionPointBrowser.class));
					break;
				case ACTIVITY_DASHBOARDS:
					Log.d("", "ACTIVITY_DASHBOARDS");
					startActivity(new Intent(this, DashboardBrowser.class));
				default:
					break;
			}
		else
			showToast(getString(R.string.notify_disconnected));
	}

	/**
	 * @param text
	 */
	public void setStatusText(String text, int color)
	{
		statusText.setTextColor(color);
		statusText.setText(text);
	}

	public void showToast(final String text)
	{
		new Handler(getMainLooper()).post(new Runnable() {
			@Override
			public void run()
			{
				Toast.makeText(getApplicationContext(), text, Toast.LENGTH_SHORT).show();
			}
		});
	}
}
