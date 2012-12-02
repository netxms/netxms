package org.netxms.ui.android.receiver;

import org.netxms.ui.android.service.ClientConnectorService;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
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
		Bundle bundle = intent.getExtras();
		Intent i = new Intent(context, ClientConnectorService.class);
		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(context);
		String action = ClientConnectorService.ACTION_CONNECT;
		if (sp.getBoolean("global.scheduler.enable", false))
			action = bundle.getString("action");
		i.setAction(action);
		context.startService(i);
	}
}
