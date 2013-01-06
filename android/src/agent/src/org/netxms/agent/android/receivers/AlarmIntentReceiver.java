package org.netxms.agent.android.receivers;

import org.netxms.agent.android.main.activities.HomeScreen;
import org.netxms.agent.android.service.ClientConnectorService;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

/**
 * Intent receiver for timer (alarm) broadcast message
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class AlarmIntentReceiver extends BroadcastReceiver
{
	@Override
	public void onReceive(Context context, Intent intent)
	{
		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(context);
		if (!sp.getBoolean(HomeScreen.INTENTIONAL_EXIT_KEY, false) && sp.getBoolean("global.activate", false))
		{
			Intent i = new Intent(context, ClientConnectorService.class);
			i.setAction(ClientConnectorService.ACTION_CONNECT);
			context.startService(i);
		}
	}
}
