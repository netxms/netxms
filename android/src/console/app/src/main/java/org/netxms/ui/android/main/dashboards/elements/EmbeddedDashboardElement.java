/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.concurrent.ScheduledExecutorService;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.android.main.dashboards.configs.EmbeddedDashboardConfig;
import org.netxms.ui.android.main.views.DashboardView;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.util.Log;
import android.view.Gravity;

/**
 * Embedded dashboard element for dashboard
 */
public class EmbeddedDashboardElement extends AbstractDashboardElement
{
	private static final String TAG = "nxclient/EmbeddedDashboardElement";

	private EmbeddedDashboardConfig config;
	private int current = 0;

	/**
	 * @param context
	 * @param xmlConfig
	 * @param service
	 */
	public EmbeddedDashboardElement(Context context, String xmlConfig, ClientConnectorService service, ScheduledExecutorService scheduleTaskExecutor)
	{
		super(context, xmlConfig, service, scheduleTaskExecutor);

		try
		{
			config = EmbeddedDashboardConfig.createFromXml(xmlConfig);
		}
		catch(Exception e)
		{
			Log.e(TAG, "Error parsing element config", e);
			config = new EmbeddedDashboardConfig();
		}
		
		setPadding(0, 0, 0, 0);
	}

	/* (non-Javadoc)
	 * @see android.view.View#onAttachedToWindow()
	 */
	@Override
	protected void onAttachedToWindow()
	{
		super.onAttachedToWindow();
		scheduleTaskExecutor.execute(new Runnable() {
			@Override
			public void run()
			{
				try
				{
					service.getSession().syncMissingObjects(config.getDashboardObjects(), false);
				}
				catch(Exception e)
				{
					Log.e(TAG, "syncMissingObjects() failed", e);
				}
				if (config.getDashboardObjects().length > 1)
					startRefreshTask(config.getDisplayInterval());
				else
					refresh();
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.dashboards.elements.AbstractDashboardElement#refresh()
	 */
	@Override
	public void refresh()
	{
		final Dashboard dashboard = (Dashboard)service.getSession().findObjectById(config.getDashboardObjects()[current], Dashboard.class);
		current++;
		if (current >= config.getDashboardObjects().length)
			current = 0;
		
		post(new Runnable() {
			@Override
			public void run()
			{
				removeAllViews();
				if (dashboard != null)
				{
					DashboardView view = new DashboardView(getContext(), dashboard, service, scheduleTaskExecutor);
					addView(view, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT, Gravity.FILL));
				}
			}
		});
	}
}
