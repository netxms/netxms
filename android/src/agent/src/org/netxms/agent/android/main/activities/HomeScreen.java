package org.netxms.agent.android.main.activities;

import java.util.Calendar;

import org.netxms.agent.android.R;
import org.netxms.base.NXCommon;

import android.content.ComponentName;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
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
	private TextView lastConnText;
	private TextView nextConnText;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	public void onCreateStep2(Bundle savedInstanceState)
	{
		setIntentionalExit(false); // Allow autorestart on change connectivity status for premature exit
		setContentView(R.layout.homescreen);

		sp = PreferenceManager.getDefaultSharedPreferences(this);

		agentStatusText = (TextView)findViewById(R.id.AgentStatus);
		lastConnText = (TextView)findViewById(R.id.LastConnection);
		nextConnText = (TextView)findViewById(R.id.NextConnection);
		TextView buildName = (TextView)findViewById(R.id.MainScreenVersion);
		buildName.setText(getString(R.string.version, getAppVersion(), NXCommon.VERSION, getString(R.string.build_number)));
		refreshStatus();
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
			if (service != null)
				service.reconnect(true);
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
	 */
	@SuppressWarnings("deprecation")
	public void refreshStatus()
	{
		long last = sp.getLong("global.scheduler.last_activation", 0);
		if (last != 0)
		{
			Calendar cal = Calendar.getInstance(); // get a Calendar object with current time
			cal.setTimeInMillis(last);
			lastConnText.setText(
					getString(R.string.info_last_connection,
							cal.getTime().toLocaleString(),
							sp.getString("global.scheduler.last_activation_msg",
									getString(R.string.ok))));
		}
		if (sp.getBoolean("global.activate", false))
		{
			String status = getString(R.string.pref_global_activate_enabled);
			String minutes = sp.getString("global.scheduler.interval", "15");
			String range = "";
			if (sp.getBoolean("global.scheduler.daily.enable", false))
				range = getString(R.string.info_agent_range, sp.getString("global.scheduler.daily.on", "00:00"), sp.getString("global.scheduler.daily.off", "00:00"));
			else
				range = getString(R.string.info_agent_whole_day);
			agentStatusText.setText(getString(R.string.info_agent_status, status, minutes, range));
			long next = sp.getLong("global.scheduler.next_activation", 0);
			if (next != 0)
			{
				Calendar cal = Calendar.getInstance(); // get a Calendar object with current time
				cal.setTimeInMillis(next);
				nextConnText.setText(getString(R.string.info_next_connection, cal.getTime().toLocaleString()));
			}
		}
		else
		{
			agentStatusText.setText(getString(R.string.pref_global_activate_disabled));
			nextConnText.setText(getString(R.string.info_next_connection_unscheduled));
		}
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

	/**
	 * Get app version as defined in manifest
	 * 
	 */
	private String getAppVersion()
	{
		String version = "";
		try
		{
			version = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
		}
		catch (PackageManager.NameNotFoundException e)
		{
			e.printStackTrace();
		}
		return version;
	}

}
