package org.netxms.agent.android.receivers;

import org.netxms.agent.android.service.AgentConnectorService;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

/**
 * Intent receiver for boot completed broadcast message
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class BootCompletedIntentReceiver extends BroadcastReceiver
{
	@Override
	public void onReceive(Context context, Intent intent)
	{
		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(context);
		if (sp.getBoolean("global.autostart", false) && sp.getBoolean("global.activate", false))
		{
			Intent serviceIntent = new Intent(context, AgentConnectorService.class);
			context.startService(serviceIntent);
		}
	}
}
