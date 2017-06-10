package org.netxms.agent.android.main.activities;

import org.netxms.agent.android.R;
import org.netxms.agent.android.helpers.DeviceInfoHelper;
import org.netxms.agent.android.helpers.SafeParser;
import org.netxms.agent.android.helpers.SharedPrefs;
import org.netxms.agent.android.helpers.TimeHelper;
import org.netxms.agent.android.service.AgentConnectorService;
import org.netxms.base.NXCommon;

import android.Manifest;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.telephony.TelephonyManager;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;
import android.widget.Toast;

import static org.netxms.agent.android.helpers.DeviceInfoHelper.getDeviceId;
import static org.netxms.agent.android.service.AgentConnectorService.LOCATION_LAST_STATUS;
import static org.netxms.agent.android.service.AgentConnectorService.PERMISSIONS_GRANTED;
import static org.netxms.agent.android.service.AgentConnectorService.SCHEDULER_LAST_ACTIVATION;
import static org.netxms.agent.android.service.AgentConnectorService.SCHEDULER_LAST_ACTIVATION_MSG;
import static org.netxms.agent.android.service.AgentConnectorService.SCHEDULER_NEXT_ACTIVATION;

/**
 * Home screen activity
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class HomeScreen extends AbstractClientActivity
{
	public static final String INTENTIONAL_EXIT_KEY = "IntentionalExit";
	private static final String[] TAG_STRING_PERMISSIONS = new String[]
		{
			Manifest.permission.ACCESS_FINE_LOCATION,
			Manifest.permission.READ_PHONE_STATE,
			Manifest.permission.GET_ACCOUNTS
		};

	private static final int TAG_CODE_PERMISSIONS = 1;

	private SharedPreferences sp;
	private TextView agentStatusText;
	private TextView locationStatusText;
	private TextView lastConnText;
	private TextView nextConnText;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.agent.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	public void onCreateStep2(Bundle savedInstanceState)
	{
		sp = PreferenceManager.getDefaultSharedPreferences(this);
		SharedPrefs.writeBoolean(sp, INTENTIONAL_EXIT_KEY, false);	// Allow autorestart on change connectivity status for premature exit
		setContentView(R.layout.homescreen);

		agentStatusText = (TextView)findViewById(R.id.AgentStatus);
		locationStatusText = (TextView)findViewById(R.id.LastLocation);
		lastConnText = (TextView)findViewById(R.id.LastConnection);
		nextConnText = (TextView)findViewById(R.id.NextConnection);
		TextView buildName = (TextView)findViewById(R.id.MainScreenVersion);
		buildName.setText(getString(R.string.version) + " " + NXCommon.VERSION + " (" + getString(R.string.build_number) + ")");

		// Check for permissions and request as necessary
		if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED ||
				ContextCompat.checkSelfPermission(this, Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED ||
				ContextCompat.checkSelfPermission(this, Manifest.permission.GET_ACCOUNTS) != PackageManager.PERMISSION_GRANTED)
		{
			SharedPrefs.writeBoolean(sp, PERMISSIONS_GRANTED, false);
			ActivityCompat.requestPermissions(this, TAG_STRING_PERMISSIONS, TAG_CODE_PERMISSIONS);
		}
		else
			SharedPrefs.writeBoolean(sp, PERMISSIONS_GRANTED, true);
		refreshStatus();
	}

	@Override
	public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
	{
		switch (requestCode)
		{
			case TAG_CODE_PERMISSIONS:
			{
				// If request is cancelled, the result arrays are empty.
				int granted = 0;
				for (int i = 0; i < grantResults.length; i++ )
				{
					if (permissions[i] == Manifest.permission.ACCESS_FINE_LOCATION && grantResults[i] == PackageManager.PERMISSION_GRANTED)
						granted++;
					else if (permissions[i] == Manifest.permission.READ_PHONE_STATE && grantResults[i] == PackageManager.PERMISSION_GRANTED)
						granted++;
					else if (permissions[i] == Manifest.permission.GET_ACCOUNTS && grantResults[i] == PackageManager.PERMISSION_GRANTED)
						granted++;
				}
				if (granted == 3)
				{
					SharedPrefs.writeBoolean(sp, PERMISSIONS_GRANTED, true);
					Intent i = new Intent(this, AgentConnectorService.class);
					i.setAction(AgentConnectorService.ACTION_FORCE_CONNECT);
					startService(i);
				}
				else
				{
					// Print alert!
				}
				return;
			}
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.agent.android.main.activities.AbstractClientActivity#onServiceConnected(android.content.ComponentName,
	 * android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
		service.registerHomeScreen(this);
		showDeviceInfo();
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
		menu.removeItem(android.R.id.home);
		menu.add(Menu.NONE, R.string.reconnect, Menu.NONE, getString(R.string.reconnect)).setIcon(android.R.drawable.ic_menu_revert);
		menu.add(Menu.NONE, R.string.exit, Menu.NONE, getString(R.string.exit)).setIcon(android.R.drawable.ic_menu_close_clear_cancel);
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
		if (item.getItemId() == R.string.reconnect)
		{
			Intent i = new Intent(this, AgentConnectorService.class);
			i.setAction(AgentConnectorService.ACTION_FORCE_CONNECT);
			startService(i);
			return true;
		}
		else if (item.getItemId() == R.string.exit)
		{
			if (service != null)
				service.shutdown();
			SharedPrefs.writeBoolean(sp, INTENTIONAL_EXIT_KEY, true);	 // Avoid autorestart on change connectivity status for intentional exit
			moveTaskToBack(true);
			System.exit(0);
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	/**
	 * Refresh device status info (agent status and schedules)
	 */
	public void refreshStatus()
	{
		long last = sp.getLong(SCHEDULER_LAST_ACTIVATION, 0);
		if (last != 0)
			lastConnText.setText(getString(R.string.info_agent_last_connection,
					getString(R.string.info_agent_connection_attempt,
							TimeHelper.getTimeString(last),
							sp.getString(SCHEDULER_LAST_ACTIVATION_MSG, getString(R.string.ok)))));
		if (sp.getBoolean(PERMISSIONS_GRANTED, false))
		{
			((TextView)findViewById(R.id.Permissions)).setText(getString(R.string.info_granted_permissions));
			if (sp.getBoolean("global.activate", false))
			{
				String status = getString(R.string.pref_global_activate_enabled);
				String minutes = sp.getString("connection.interval", "15");
				String range = "";
				if (sp.getBoolean("scheduler.daily.enable", false))
					range = getString(R.string.info_agent_range, sp.getString("scheduler.daily.on", "00:00"), sp.getString("scheduler.daily.off", "00:00"));
				else
					range = getString(R.string.info_agent_whole_day);
				String location = "";
				if (sp.getBoolean("location.force", false))
				{
					String interval = sp.getString("location.interval", "30");
					String duration = sp.getString("location.duration", "2");
					String strategies[] = getResources().getStringArray(R.array.locations_strategy_labels);
					String strategy = "";
					int type = SafeParser.parseInt(sp.getString("location.strategy", "0"), 0);
					if (type < strategies.length)
						strategy = strategies[type];
					location = getString(R.string.info_agent_location_active, interval, duration, strategy);
				}
				else
					location = getString(R.string.info_agent_location_passive);
				agentStatusText.setText(getString(R.string.info_agent_status, status, minutes, range, location));
				locationStatusText.setText(getString(R.string.info_agent_last_location, sp.getString(LOCATION_LAST_STATUS, "")));
				long next = sp.getLong(SCHEDULER_NEXT_ACTIVATION, 0);
				if (next != 0)
					nextConnText.setText(getString(R.string.info_agent_next_connection, getString(R.string.info_agent_connection_scheduled, TimeHelper.getTimeString(next))));
			}
			else
			{
				agentStatusText.setText(getString(R.string.pref_global_activate_disabled));
				locationStatusText.setText("");
				nextConnText.setText(getString(R.string.info_agent_next_connection, getString(R.string.info_agent_connection_unscheduled)));
			}
		}
		else	// Notify permissions not granted!
		{
			((TextView)findViewById(R.id.Permissions)).setText(getString(R.string.info_missing_permissions));
		}
	}
	/**
	 * Refresh device status info (agent status and schedules)
	 */
	public void showDeviceInfo()
	{
		((TextView)findViewById(R.id.Imei)).setText(getString(R.string.info_device_id, getDeviceId(getApplicationContext())));
		if (DeviceInfoHelper.getSerial().length() > 0)
			((TextView)findViewById(R.id.Serial)).setText(getString(R.string.info_device_serial, DeviceInfoHelper.getSerial()));
	}

	public void showToast(final String text)
	{
		new Handler(getMainLooper()).post(new Runnable()
		{
			@Override
			public void run()
			{
				Toast.makeText(getApplicationContext(), text, Toast.LENGTH_SHORT).show();
			}
		});
	}
}
