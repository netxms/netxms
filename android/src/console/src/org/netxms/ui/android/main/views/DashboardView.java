/**
 * 
 */
package org.netxms.ui.android.main.views;

import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.android.main.dashboards.configs.DashboardElementLayout;
import org.netxms.ui.android.main.dashboards.elements.AbstractDashboardElement;
import org.netxms.ui.android.main.dashboards.elements.BarChartElement;
import org.netxms.ui.android.main.dashboards.elements.DialChartElement;
import org.netxms.ui.android.main.dashboards.elements.LabelElement;
import org.netxms.ui.android.main.dashboards.elements.LineChartElement;
import org.netxms.ui.android.main.dashboards.elements.PieChartElement;
import org.netxms.ui.android.main.dashboards.elements.TableBarChartElement;
import org.netxms.ui.android.main.dashboards.elements.TablePieChartElement;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.util.Log;

/**
 * Dashboard view
 */
public class DashboardView extends DashboardLayout
{
	private ClientConnectorService service;

	/**
	 * @param context
	 * @param dashboard
	 */
	public DashboardView(Context context, Dashboard dashboard, ClientConnectorService service)
	{
		super(context);
		this.service = service;

		setColumnCount(dashboard.getNumColumns());

		for(DashboardElement e : dashboard.getElements())
		{
			createElement(e);
		}
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
			case DashboardElement.LINE_CHART:
				widget = new LineChartElement(getContext(), element.getData(), service);
				break;
			case DashboardElement.BAR_CHART:
			case DashboardElement.TUBE_CHART:
				widget = new BarChartElement(getContext(), element.getData(), service);
				break;
			case DashboardElement.PIE_CHART:
				widget = new PieChartElement(getContext(), element.getData(), service);
				break;
			case DashboardElement.DIAL_CHART:
				widget = new DialChartElement(getContext(), element.getData(), service);
				break;
			case DashboardElement.TABLE_BAR_CHART:
			case DashboardElement.TABLE_TUBE_CHART:
				widget = new TableBarChartElement(getContext(), element.getData(), service);
				break;
			case DashboardElement.TABLE_PIE_CHART:
				widget = new TablePieChartElement(getContext(), element.getData(), service);
				break;
			case DashboardElement.LABEL:
				widget = new LabelElement(getContext(), element.getData(), service);
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

		DashboardLayout.LayoutParams layoutParams = new DashboardLayout.LayoutParams(layout.horizontalSpan, layout.verticalSpan);
		widget.setLayoutParams(layoutParams);
		addView(widget);
	}
}
