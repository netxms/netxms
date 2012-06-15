/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import org.netxms.ui.android.main.dashboards.configs.LabelConfig;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.util.Log;
import android.view.Gravity;
import android.widget.TextView;

/**
 * Label element for dashboard
 */
public class LabelElement extends AbstractDashboardElement
{
	private static final String LOG_TAG = "nxclient/LabelElement";
	
	/**
	 * @param context
	 * @param xmlConfig
	 * @param service
	 */
	public LabelElement(Context context, String xmlConfig, ClientConnectorService service)
	{
		super(context, xmlConfig, service);

		LabelConfig config;
		try
		{
			config = LabelConfig.createFromXml(xmlConfig);
		}
		catch(Exception e)
		{
			Log.e(LOG_TAG, "Error parsing element config", e);
			config = new LabelConfig();
		}
		
		TextView view = new TextView(context);
		view.setText(config.getTitle());
		view.setGravity(Gravity.FILL);
		view.setBackgroundColor(toAndroidColor(config.getBackgroundColorAsInt()));
		view.setTextColor(toAndroidColor(config.getForegroundColorAsInt()));
		
		addView(view);
	}
}
