package org.netxms.agent.android.main.settings;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.preference.SwitchPreference;
import android.util.AttributeSet;

/**
 * Custom handler for switch preference to handle bug (reset of switch when going out of screen)
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

@TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
public class CustomSwitchPreference extends SwitchPreference
{

	public CustomSwitchPreference(Context context)
	{
		this(context, null);
	}

	public CustomSwitchPreference(Context context, AttributeSet attrs)
	{
		this(context, attrs, android.R.attr.switchPreferenceStyle);
	}

	public CustomSwitchPreference(Context context, AttributeSet attrs, int defStyle)
	{
		super(context, attrs, defStyle);
	}
}
