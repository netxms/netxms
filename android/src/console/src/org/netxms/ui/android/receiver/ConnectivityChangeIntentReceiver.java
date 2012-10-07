package org.netxms.ui.android.receiver;

import org.netxms.ui.android.main.activities.HomeScreen;
import org.netxms.ui.android.service.ClientConnectorService;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.preference.PreferenceManager;

/**
 * Intent receiver for connectivity change broadcast message
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class ConnectivityChangeIntentReceiver extends BroadcastReceiver
{
	@Override
	public void onReceive(Context context, Intent intent)
	{
		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(context);
		if (!sp.getBoolean(HomeScreen.INTENTIONAL_EXIT_KEY, false) && !sp.getBoolean("global.scheduler.enable", false))
			if (intent.getExtras() != null)
			{
				NetworkInfo ni = (NetworkInfo)intent.getExtras().get(ConnectivityManager.EXTRA_NETWORK_INFO);
				if (ni != null && ni.getState() == NetworkInfo.State.CONNECTED)
				{
					Intent i = new Intent(context, ClientConnectorService.class);
					i.setAction(ClientConnectorService.ACTION_RECONNECT);
					context.startService(i);
				}
				if (intent.getExtras().getBoolean(ConnectivityManager.EXTRA_NO_CONNECTIVITY, Boolean.FALSE))
				{
					Intent i = new Intent(context, ClientConnectorService.class);
					i.setAction(ClientConnectorService.ACTION_DISCONNECT);
					context.startService(i);
				}
			}
	}
}
