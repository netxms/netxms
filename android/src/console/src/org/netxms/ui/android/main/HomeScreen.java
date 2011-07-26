package org.netxms.ui.android.main;

import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.ActivityListAdapter;
import org.netxms.ui.android.service.ClientConnectorService;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.TextView;

public class HomeScreen extends Activity implements OnItemClickListener, ServiceConnection
{
	public static final int ACTIVITY_ALARMS = 1;
	public static final int ACTIVITY_DASHBOARDS = 2;
	
	private ClientConnectorService service;
	private TextView statusText; 
	
	/* (non-Javadoc)
	 * @see android.app.Activity#onCreate(android.os.Bundle)
	 */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.homescreen);

		startService(new Intent(this, ClientConnectorService.class));
		bindService(new Intent(this, ClientConnectorService.class), this, 0);

		GridView gridview = (GridView)findViewById(R.id.ActivityList);
		gridview.setAdapter(new ActivityListAdapter(this));

		gridview.setOnItemClickListener(this);

		statusText = (TextView)findViewById(R.id.MainScreenStatus);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
	    MenuInflater inflater = getMenuInflater();
	    inflater.inflate(R.menu.main_menu, menu);
	    return true;
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch(item.getItemId())
		{
			case R.id.settings:
				startActivity(new Intent(this, ConsolePreferences.class));
				return true;
			case R.id.disconnect:
				if (!(service == null)) 
				{
					service.clearNotifications();
					service.stopSelf();
					System.exit(0);
				}
				return true;
			default:
				return super.onOptionsItemSelected(item);
		}
	}

	/* (non-Javadoc)
	 * @see android.widget.AdapterView.OnItemClickListener#onItemClick(android.widget.AdapterView, android.view.View, int, long)
	 */
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id)
	{
		switch((int)id)
		{
			case ACTIVITY_ALARMS:
				startActivity(new Intent(this, AlarmBrowser.class));
				break;
			default:
				break;
		}
	}
	
	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		service = ((ClientConnectorService.ClientConnectorBinder)binder).getService();
		// remember this activity in service, so that service can send updates
		service.registerHomeScreen(this);
		statusText.setText(service.getConnectionStatus());
	}

	/* (non-Javadoc)
	 * @see android.content.ServiceConnection#onServiceDisconnected(android.content.ComponentName)
	 */
	@Override
	public void onServiceDisconnected(ComponentName name)
	{
	}
	
	/**
	 * @param text
	 */
	public void setStatusText(String text)
	{
		statusText.setText(text);
	}
}
