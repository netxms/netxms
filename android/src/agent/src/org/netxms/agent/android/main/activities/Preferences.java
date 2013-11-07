/**
 * 
 */
package org.netxms.agent.android.main.activities;

import org.netxms.agent.android.R;
import org.netxms.agent.android.service.AgentConnectorService;

import android.content.Intent;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;

/**
 * Console preferences
 *
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 *
 */

public class Preferences extends PreferenceActivity
{
	/* (non-Javadoc)
	 * @see android.preference.PreferenceActivity#onCreate(android.os.Bundle)
	 */
	@SuppressWarnings("deprecation")
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.preferences);
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
		Intent i = new Intent(this, AgentConnectorService.class);
		i.setAction(AgentConnectorService.ACTION_CONFIGURE);
		startService(i);
		super.onDestroy();
	}

	@SuppressWarnings("deprecation")
	@Override
	public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference)
	{
		super.onPreferenceTreeClick(preferenceScreen, preference);
		if (preference != null)
			if (preference instanceof PreferenceScreen)
				if (((PreferenceScreen)preference).getDialog() != null)
					((PreferenceScreen)preference).getDialog().getWindow().getDecorView().setBackgroundDrawable(this.getWindow().getDecorView().getBackground().getConstantState().newDrawable());
		return false;
	}
}
