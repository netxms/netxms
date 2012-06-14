/**
 * 
 */
package org.netxms.ui.android.main.views;

import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.android.main.dashboards.configs.DashboardElementLayout;
import org.netxms.ui.android.main.dashboards.elements.AbstractDashboardElement;
import org.netxms.ui.android.main.dashboards.elements.BarChartElement;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.graphics.Color;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.LinearLayout;

/**
 * Dashboard view
 */
public class DashboardView extends LinearLayout
{
	private ClientConnectorService service;
	private Dashboard dashboard;

	/**
	 * @param context
	 * @param dashboard
	 */
	public DashboardView(Context context, Dashboard dashboard, ClientConnectorService service)
	{
		super(context);
		this.service = service;
		this.dashboard = dashboard;
		
		setPadding(5, 5, 5, 5);
		//setBackgroundColor(Color.WHITE);
		
		//setNumColumns(dashboard.getNumColumns());
		for(DashboardElement e : dashboard.getElements())
			createElement(e);
	}

	/**
	 * @param element
	 */
	private void createElement(DashboardElement element)
	{
		Log.d("DashboardView", "createElement: type=" + element.getType());
		
		AbstractDashboardElement widget;
		switch(element.getType())
		{
			case DashboardElement.BAR_CHART:
				widget = new BarChartElement(getContext(), element.getData(), service);
				break;
			default:
				widget = new AbstractDashboardElement(getContext(), element.getData(), service);
				break;
		}
		
		DashboardElementLayout layout;
		try
		{
			layout = DashboardElementLayout.createFromXml(element.getLayout());
		}
		catch(Exception e)
		{
			Log.e("DashboardView/createElement", "Error parsing element layout", e);
			layout = new DashboardElementLayout();
		}
		
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
		addView(widget, lp);
	}
}
