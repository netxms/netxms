/**
 * 
 */
package org.netxms.ui.android.main.views;

import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.android.main.dashboards.configs.DashboardElementLayout;
import org.netxms.ui.android.main.dashboards.elements.AbstractDashboardElement;
import org.netxms.ui.android.main.dashboards.elements.BarChartElement;
import android.content.Context;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Dashboard view
 */
public class DashboardView extends LinearLayout
{
	private Dashboard dashboard;

	/**
	 * @param context
	 * @param dashboard
	 */
	public DashboardView(Context context, Dashboard dashboard)
	{
		super(context);
		this.dashboard = dashboard;
		
		setPadding(5, 5, 5, 5);
		setBackgroundColor(0xFF800080);
		
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
				widget = new BarChartElement(getContext(), element.getData());
				break;
			default:
				widget = new AbstractDashboardElement(getContext(), element.getData());
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
		
		LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT);
		addView(widget, lp);
	}
}
