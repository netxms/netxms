/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.List;

import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;

import android.annotation.TargetApi;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceActivity;

/**
 * Console preferences
 *
 */
public class ConsolePreferences extends PreferenceActivity
{
	/* (non-Javadoc)
	 * @see android.preference.PreferenceActivity#onCreate(android.os.Bundle)
	 */
	@SuppressWarnings("deprecation")
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		String action = getIntent().getAction();
		if (action != null && action.equals("org.netxms.ui.android.ConsolePreferences.global"))
		{
			addPreferencesFromResource(R.xml.preference_global);
		}
		else if (action != null && action.equals("org.netxms.ui.android.ConsolePreferences.connection"))
		{
			addPreferencesFromResource(R.xml.preference_connection);
		}
		else if (action != null && action.equals("org.netxms.ui.android.ConsolePreferences.notifications"))
		{
			addPreferencesFromResource(R.xml.preference_notifications);
		}
		else if (action != null && action.equals("org.netxms.ui.android.ConsolePreferences.interface"))
		{
			addPreferencesFromResource(R.xml.preference_interface);
		}
		else if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB)
		{	// Load the legacy preferences headers
			addPreferencesFromResource(R.xml.preference_headers_legacy);
		}
	}

	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
	@Override
	public void onBuildHeaders(List<Header> target)
	{
		loadHeadersFromResource(R.xml.preference_headers, target);
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onPause()
	 */
	@Override
	protected void onPause()
	{
		super.onPause();

	}

	/* (non-Javadoc)
	 * @see android.preference.PreferenceActivity#onDestroy()
	 */
	@Override
	protected void onDestroy()
	{
		Intent i = new Intent(this, ClientConnectorService.class);
		i.setAction(ClientConnectorService.ACTION_CONFIGURE);
		startService(i);
		super.onDestroy();
	}
}
