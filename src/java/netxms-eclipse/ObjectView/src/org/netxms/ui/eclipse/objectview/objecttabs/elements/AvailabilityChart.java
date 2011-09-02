/**
 * 
 */
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.charts.widgets.DataComparisonBirtChart;

/**
 * Availability chart for business services
 */
public class AvailabilityChart extends OverviewPageElement
{
	DataComparisonBirtChart dayChart;
	DataComparisonBirtChart weekChart;
	DataComparisonBirtChart monthChart;
	
	/**
	 * @param parent
	 * @param object
	 */
	public AvailabilityChart(Composite parent, GenericObject object)
	{
		super(parent, object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return "Availability";
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	void onObjectChange()
	{
		BusinessService service = (BusinessService)getObject();
		
		dayChart.updateParameter(0, service.getUptimeForDay(), false);
		dayChart.updateParameter(1, 100.0 - service.getUptimeForDay(), true);

		weekChart.updateParameter(0, service.getUptimeForWeek(), false);
		weekChart.updateParameter(1, 100.0 - service.getUptimeForWeek(), true);
		
		monthChart.updateParameter(0, service.getUptimeForMonth(), false);
		monthChart.updateParameter(1, 100.0 - service.getUptimeForMonth(), true);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		Composite clientArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		clientArea.setLayout(layout);
		
		dayChart = createChart(clientArea, "Today");
		weekChart = createChart(clientArea, "This Week");
		monthChart = createChart(clientArea, "This Month");
		
		return clientArea;
	}
	
	/**
	 * @param parent
	 * @param title
	 * @return
	 */
	private DataComparisonBirtChart createChart(Composite parent, String title)
	{
		DataComparisonBirtChart chart = new DataComparisonBirtChart(parent, SWT.NONE, DataComparisonChart.PIE_CHART);
		chart.setTitleVisible(true);
		chart.set3DModeEnabled(true);
		chart.setChartTitle(title);
		chart.setLegendVisible(false);
		chart.setLabelsVisible(false);
		chart.setRotation(225.0);
		
		chart.addParameter(new GraphItem(0, 0, 0, 0, "Up", "Up"), 100);
		chart.addParameter(new GraphItem(0, 0, 0, 0, "Down", "Down"), 0);
		chart.setPaletteEntry(0, new ChartColor(127, 154, 72));
		chart.setPaletteEntry(1, new ChartColor(158, 65, 62));
		chart.initializationComplete();
		
		GridData gd = new GridData();
		gd.widthHint = 250;
		gd.heightHint = 190;
		chart.setLayoutData(gd);
		return chart;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public boolean isApplicableForObject(GenericObject object)
	{
		return object instanceof BusinessService;
	}
}
