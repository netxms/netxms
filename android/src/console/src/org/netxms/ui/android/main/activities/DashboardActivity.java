/**
 * 
 */
package org.netxms.ui.android.main.activities;

import org.netxms.client.objects.Dashboard;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.views.DashboardView;
import android.content.ComponentName;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.FrameLayout;

/**
 * Dashboard activity
 */
public class DashboardActivity extends AbstractClientActivity
{
	private long dashboardId;
	private Dashboard dashboard;
	private FrameLayout rootView;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		dashboardId = getIntent().getLongExtra("objectId", 0);
		Log.d("DashboardActivity", "onCreateStep2: dashboardId=" + dashboardId);
		setContentView(R.layout.dashboard);
		rootView = (FrameLayout)findViewById(R.id.DashboardLayout);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
		dashboard = (Dashboard)service.findObjectById(dashboardId, Dashboard.class);
		refresh();
	}
	
	/* (non-Javadoc)
	 * @see android.app.Activity#onStop()
	 */
	@Override
	protected void onStop()
	{
		rootView.removeAllViews();
		super.onStop();
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onRestart()
	 */
	@Override
	protected void onRestart()
	{
		super.onRestart();
		refresh();
	}

	/**
	 * Refresh dashboard
	 */
	private void refresh()
	{
		rootView.addView(new DashboardView(this, dashboard, service), new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT));
	}
}
