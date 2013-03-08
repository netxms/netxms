/**
 * 
 */
package org.netxms.ui.android.main.fragments;

import java.util.ArrayList;

import org.netxms.ui.android.R;

import android.content.ComponentName;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;

/**
 * Alarm browser in fragment style
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 *
 */
public class AlarmBrowserFragment extends AbstractFragmentActivity
{
	private AlarmsFragment fragment = null;
	private ArrayList<Integer> nodeIdList;

	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		setContentView(R.layout.fragment_container);
		nodeIdList = getIntent().getIntegerArrayListExtra("nodeIdList");
		if (findViewById(R.id.fragment_container) != null)
		{
			if (savedInstanceState == null)
			{
				FragmentManager fragmentManager = getSupportFragmentManager();
				FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction();
				fragment = new AlarmsFragment();
				fragmentTransaction.add(R.id.fragment_container, fragment);
				fragmentTransaction.commit();
			}
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see android.content.ServiceConnection#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
		if (service != null && fragment != null)
		{
			fragment.setNodeIdList(nodeIdList);
			fragment.setService(service);
			fragment.refresh();
		}
	}
}
