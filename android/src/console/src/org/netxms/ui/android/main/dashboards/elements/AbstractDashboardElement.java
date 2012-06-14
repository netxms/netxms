/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import org.netxms.ui.android.main.dashboards.configs.DashboardElementConfig;
import android.content.Context;
import android.widget.FrameLayout;

/**
 * Base class for all dashboard elements
 */
public class AbstractDashboardElement extends FrameLayout
{
	protected DashboardElementConfig config;
	
	/**
	 * @param context
	 * @param xmlConfig
	 */
	public AbstractDashboardElement(Context context, String xmlConfig)
	{
		super(context);
		setBackgroundColor(0xFF800080);
	}
}
