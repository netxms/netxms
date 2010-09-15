package org.netxms.ui.eclipse.charts.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.widgets.PieChart;
import org.swtchart.IBarSeries;

public class LastValuesPieChart extends LastValuesChart
{
	public static final String ID = "org.netxms.ui.eclipse.charts.views.LastValuesPieChart";

	@Override
	protected void addItemToChart(int index, GraphItem item, double data, boolean padding)
	{
		IBarSeries series = ((PieChart) chart).addPieSeries(index, item.getDescription(), data);
		series.setBarMargin(25);
	}

	@Override
	public void createPartControl(Composite parent)
	{
		PieChart pieChart = new PieChart(parent, SWT.NONE);
		pieChart.setCategories(new String[] { "" });
		chart = pieChart;

		createActions();
		contributeToActionBars();
		createPopupMenu();

		initChart();
	}

}
