/**
 * 
 */
package org.netxms.ui.android.main.activities;

import org.netxms.ui.android.R;
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
}
