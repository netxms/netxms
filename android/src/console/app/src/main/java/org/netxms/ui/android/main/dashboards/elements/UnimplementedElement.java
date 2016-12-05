/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.concurrent.ScheduledExecutorService;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.view.Gravity;
import android.widget.TextView;

/**
 * Placeholder for unimplemented dashboard elements
 */
public class UnimplementedElement extends AbstractDashboardElement
{
	/**
	 * @param context
	 * @param xmlConfig
	 * @param service
	 */
	public UnimplementedElement(Context context, String xmlConfig, ClientConnectorService service, ScheduledExecutorService scheduleTaskExecutor)
	{
		super(context, xmlConfig, service, scheduleTaskExecutor);
		
		TextView view = new TextView(context);
		view.setText("This element type is not available on mobile devices");
		view.setGravity(Gravity.CENTER);
		addView(view, new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT, Gravity.CENTER));
	}
}
