package org.netxms.agent.android.main.activities;

import org.netxms.agent.android.R;
import org.netxms.agent.android.helpers.DeviceInfoHelper;
import org.netxms.agent.android.helpers.SafeParser;
import org.netxms.agent.android.helpers.TimeHelper;
import org.netxms.agent.android.service.AgentConnectorService;
import org.netxms.base.NXCommon;

import android.content.ComponentName;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Home screen activity
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class HomeScreen extends AbstractClientActivity
{
	public static final String INTENTIONAL_EXIT_KEY = "IntentionalExit";

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
		setIntentionalExit(false); // Allow autorestart on change connectivity status for premature exit
		setContentView(R.layout.homescreen);

		sp = PreferenceManager.getDefaultSharedPreferences(this);

		agentStatusText = (TextView)findViewById(R.id.AgentStatus);
		locationStatusText = (TextView)findViewById(R.id.LastLocation);
		lastConnText = (TextView)findViewById(R.id.LastConnection);
		nextConnText = (TextView)findViewById(R.id.NextConnection);
		TextView buildName = (TextView)findViewById(R.id.MainScreenVersion);
		buildName.setText(getString(R.string.version) + " " + NXCommon.VERSION + " (" + getString(R.string.build_number) + ")");
		refreshStatus();
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
			setIntentionalExit(true); // Avoid autorestart on change connectivity status for intentional exit
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
		long last = sp.getLong("scheduler.last_activation", 0);
		if (last != 0)
			lastConnText.setText(getString(R.string.info_agent_last_connection,
					getString(R.string.info_agent_connection_attempt,
							TimeHelper.getTimeString(last),
							sp.getString("scheduler.last_activation_msg", getString(R.string.ok)))));
		if (sp.getBoolean("global.activate", false))
		{
			String status = getString(R.string.pref_global_activate_enabled);
			String minutes = sp.getString("scheduler.interval", "15");
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
			locationStatusText.setText(getString(R.string.info_agent_last_location, sp.getString("location.last_status", "")));
			long next = sp.getLong("scheduler.next_activation", 0);
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
	/**
	 * Refresh device status info (agent status and schedules)
	 */
	public void showDeviceInfo()
	{
		((TextView)findViewById(R.id.Imei)).setText(getString(R.string.info_device_id, DeviceInfoHelper.getDeviceId(getApplicationContext())));
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

	/**
	 * Set a flag to inform about an intentional exit to avoid
	 * automatic reconnection on change connectivity status
	 * 
	 * @param flag true to signal an intentional exit
	 */
	private void setIntentionalExit(boolean flag)
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		SharedPreferences.Editor editor = prefs.edit();
		editor.putBoolean(INTENTIONAL_EXIT_KEY, flag);
		editor.commit();
	}
}
