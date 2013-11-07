package org.netxms.ui.android.main.fragments;

import org.netxms.ui.android.R;

import android.annotation.TargetApi;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceFragment;

/**
 * Fragment for preferences (API > 11)
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

@TargetApi(Build.VERSION_CODES.HONEYCOMB)
public class ConsolePreferencesFragment extends PreferenceFragment
{
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		String action = getArguments().getString("settings");
		if (action != null && action.equals("global"))
		{
			addPreferencesFromResource(R.xml.preference_global);
		}
		else if (action != null && action.equals("connection"))
		{
			addPreferencesFromResource(R.xml.preference_connection);
		}
		else if (action != null && action.equals("notifications"))
		{
			addPreferencesFromResource(R.xml.preference_notifications);
		}
		else if (action != null && action.equals("interface"))
		{
			addPreferencesFromResource(R.xml.preference_interface);
		}
	}
}
