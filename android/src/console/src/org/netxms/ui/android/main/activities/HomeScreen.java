package org.netxms.ui.android.main.activities;

import org.netxms.base.NXCommon;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.ActivityListAdapter;

import android.content.ComponentName;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.TextView;

/**
 * Home screen activity
 * 
 * @author Victor Kirhenshtein
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */
public class HomeScreen extends AbstractClientActivity implements OnItemClickListener
{
	public static final int ACTIVITY_ALARMS = 1;
	public static final int ACTIVITY_DASHBOARDS = 2;
	public static final int ACTIVITY_NODES = 3;
	public static final int ACTIVITY_GRAPHS = 4;
	public static final int ACTIVITY_MACADDRESS = 5;
	
	TextView statusText; 
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	public void onCreateStep2(Bundle savedInstanceState)
	{
		setContentView(R.layout.homescreen);

		GridView gridview = (GridView)findViewById(R.id.ActivityList);
		if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) 
			gridview.setNumColumns(4);
		else
			gridview.setNumColumns(2);
		gridview.setAdapter(new ActivityListAdapter(this));

		gridview.setOnItemClickListener(this);

		statusText = (TextView)findViewById(R.id.ScreenTitleSecondary);
		
		TextView buildName = (TextView)findViewById(R.id.MainScreenVersion);
		buildName.setText(getString(R.string.version) + " " + NXCommon.VERSION + " (" + getString(R.string.build_number) + ")");
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
		service.registerHomeScreen(this);
		statusText.setText(service.getConnectionStatus());
	}

	/* (non-Javadoc)
	 * @see android.widget.AdapterView.OnItemClickListener#onItemClick(android.widget.AdapterView, android.view.View, int, long)
	 */
	@Override
	public void onItemClick(AdapterView<?> parent, View view, int position, long id)
	{
		switch((int)id)
		{
			case ACTIVITY_ALARMS:
				startActivity(new Intent(this, AlarmBrowser.class));
				break;
			case ACTIVITY_NODES:
				startActivity(new Intent(this, NodeBrowser.class));
				break;
			case ACTIVITY_GRAPHS:
				startActivity(new Intent(this, GraphBrowser.class));
				break;
			case ACTIVITY_MACADDRESS:
				startActivity(new Intent(this, MacAddressBrowser.class));
				break;
			default:
				break;
		}
	}
	
	/**
	 * @param text
	 */
	public void setStatusText(String text)
	{
		statusText.setText(text);
	}
}
