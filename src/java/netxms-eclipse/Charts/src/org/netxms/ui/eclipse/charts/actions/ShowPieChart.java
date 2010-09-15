package org.netxms.ui.eclipse.charts.actions;

import org.netxms.ui.eclipse.charts.views.LastValuesPieChart;

public class ShowPieChart extends ShowLastValuesGraph
{

	@Override
	protected String getViewId()
	{
		return LastValuesPieChart.ID;
	}

}
