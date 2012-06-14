/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.Timer;
import java.util.TimerTask;
import org.netxms.ui.android.main.dashboards.configs.DashboardElementConfig;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.widget.FrameLayout;

/**
 * Base class for all dashboard elements
 */
public class AbstractDashboardElement extends FrameLayout
{
	protected static final int AXIS_COLOR = 0xFF000000;
	protected static final int BACKGROUND_COLOR = 0xFFF0F0F0;
	protected static final Integer[] DEFAULT_ITEM_COLORS = { 0x40699C, 0x9E413E, 0x7F9A48, 0x695185, 0x3C8DA3, 0xCC7B38, 0x4F81BD, 0xC0504D,
                                                            0x9BBB59, 0x8064A2, 0x4BACC6, 0xF79646, 0xAABAD7, 0xD9AAA9, 0xC6D6AC, 0xBAB0C9 };

	protected ClientConnectorService service;
	protected DashboardElementConfig config;
	
	private Timer timer = null;
	
	/**
	 * @param context
	 * @param xmlConfig
	 */
	public AbstractDashboardElement(Context context, String xmlConfig, ClientConnectorService service)
	{
		super(context);
		this.service = service;
	}
	
	/**
	 * Refresh dashboard element
	 */
	public void refresh()
	{
	}
	
	/**
	 * Start element auto refresh at given interval
	 * 
	 * @param interval
	 */
	protected void startRefreshTask(int interval)
	{
		if (timer != null)
			return;

		timer = new Timer(true);
		timer.schedule(new TimerTask() {
			@Override
			public void run()
			{
				refresh();
			}
		}, 0, interval * 1000);
	}
}
