/**
 * 
 */
package org.netxms.ui.android.main.views;

import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.android.main.dashboards.configs.DashboardElementLayout;
import org.netxms.ui.android.main.dashboards.elements.AbstractDashboardElement;
import org.netxms.ui.android.main.dashboards.elements.BarChartElement;
import org.netxms.ui.android.main.dashboards.elements.LineChartElement;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.graphics.Color;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.GridLayout;
import android.widget.Space;
import android.widget.TextView;

/**
 * Dashboard view
 */
public class DashboardView extends GridLayout
{
	private ClientConnectorService service;
	private Dashboard dashboard;
	private int row = 0;
	private int column = 0;

	/**
	 * @param context
	 * @param dashboard
	 */
	public DashboardView(Context context, Dashboard dashboard, ClientConnectorService service)
	{
		super(context);
		this.service = service;
		this.dashboard = dashboard;
		
		setPadding(10, 10, 10, 10);
		setAlignmentMode(ALIGN_BOUNDS);
		setRowOrderPreserved(false);

		//setBackgroundColor(Color.WHITE);
		
		//setColumnCount(dashboard.getNumColumns());
		//setRowCount(dashboard.getElements().size() / dashboard.getNumColumns());
		for(DashboardElement e : dashboard.getElements())
			createElement(e);
/*		
      Spec row1 = spec(0, 1, CENTER);
      Spec row2 = spec(1, 1, CENTER);
      Spec row3 = spec(2, BASELINE);
      Spec row4 = spec(3, BASELINE);
      Spec row5 = spec(2, 3, FILL); // allow the last two rows to overlap the middle two
      Spec row6 = spec(5);
      Spec row7 = spec(6);

      Spec col1a = spec(0, 1, CENTER);
      Spec col1b = spec(0, 1, LEFT);
      Spec col1c = spec(0, RIGHT);
      Spec col2 = spec(1, LEFT);
      Spec col3 = spec(2, FILL);
      Spec col4a = spec(3);
      Spec col4b = spec(3, FILL);

      {
          TextView c = new TextView(context);
          c.setTextSize(32);
          c.setText("Email setup");
          addView(c, new LayoutParams(row1, col1a));
      }
      {
          TextView c = new TextView(context);
          c.setTextSize(16);
          c.setText("You can configure email in just a few steps:");
          addView(c, new LayoutParams(row2, col1b));
      }
      */
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

		LayoutParams lp = new LayoutParams();
//		lp.rowSpec = spec(row, layout.verticalSpan, CENTER);
//		lp.columnSpec = spec(column, layout.horizontalSpan, CENTER);
		lp.rowSpec = spec(row++, 1, LEFT);
		lp.columnSpec = spec(0, 1, RIGHT);
 		addView(widget);
		
		column += layout.horizontalSpan;
		if (column >= getColumnCount())
		{
			column = 0;
			row++;
		}
	}
}
