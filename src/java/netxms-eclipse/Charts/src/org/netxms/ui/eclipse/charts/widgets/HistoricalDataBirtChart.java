/**
 * 
 */
package org.netxms.ui.eclipse.charts.widgets;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.eclipse.birt.chart.model.Chart;
import org.eclipse.birt.chart.model.ChartWithAxes;
import org.eclipse.birt.chart.model.attribute.AxisType;
import org.eclipse.birt.chart.model.attribute.ChartDimension;
import org.eclipse.birt.chart.model.attribute.LegendItemType;
import org.eclipse.birt.chart.model.attribute.LineStyle;
import org.eclipse.birt.chart.model.attribute.Text;
import org.eclipse.birt.chart.model.attribute.impl.ColorDefinitionImpl;
import org.eclipse.birt.chart.model.attribute.impl.LineAttributesImpl;
import org.eclipse.birt.chart.model.component.Axis;
import org.eclipse.birt.chart.model.component.Series;
import org.eclipse.birt.chart.model.component.impl.SeriesImpl;
import org.eclipse.birt.chart.model.data.DateTimeDataSet;
import org.eclipse.birt.chart.model.data.NumberDataSet;
import org.eclipse.birt.chart.model.data.SeriesDefinition;
import org.eclipse.birt.chart.model.data.TextDataSet;
import org.eclipse.birt.chart.model.data.impl.DateTimeDataSetImpl;
import org.eclipse.birt.chart.model.data.impl.NumberDataSetImpl;
import org.eclipse.birt.chart.model.data.impl.SeriesDefinitionImpl;
import org.eclipse.birt.chart.model.data.impl.TextDataSetImpl;
import org.eclipse.birt.chart.model.impl.ChartWithAxesImpl;
import org.eclipse.birt.chart.model.type.LineSeries;
import org.eclipse.birt.chart.model.type.impl.LineSeriesImpl;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.ui.eclipse.charts.api.HistoricalDataChart;

/**
 * BIRT-based implementation of historical data chart
 *
 */
public class HistoricalDataBirtChart extends GenericBirtChart implements HistoricalDataChart
{
	private List<GraphItem> items = new ArrayList<GraphItem>(MAX_CHART_ITEMS);
	private List<DciData> itemData = new ArrayList<DciData>(MAX_CHART_ITEMS);
	private Axis xAxis = null;
	private Axis yAxis = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public HistoricalDataBirtChart(Composite parent, int style)
	{
		super(parent, style);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#hasAxes()
	 */
	@Override
	public boolean hasAxes()
	{
		return true;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#setTimeRange(java.util.Date, java.util.Date)
	 */
	@Override
	public void setTimeRange(Date from, Date to)
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#addParameter(org.netxms.client.datacollection.GraphItem)
	 */
	@Override
	public int addParameter(GraphItem item)
	{
		items.add(item);
		itemData.add(null);
		return items.size() - 1;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.HistoricalDataChart#updateParameter(int, org.netxms.client.datacollection.DciData, boolean)
	 */
	@Override
	public void updateParameter(int index, DciData data, boolean updateChart)
	{
		try
		{
			GraphItem item = items.get(index);
			itemData.set(index, data);
			if (updateChart)
				refresh();
		}
		catch(IndexOutOfBoundsException e)
		{
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#refresh()
	 */
	@Override
	public void refresh()
	{
		super.refresh();
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.widgets.GenericBirtChart#createChart()
	 */
	@Override
	protected Chart createChart()
	{
		ChartWithAxes chart = ChartWithAxesImpl.create();
		chart.setDimension(is3DModeEnabled() ? ChartDimension.TWO_DIMENSIONAL_WITH_DEPTH_LITERAL : ChartDimension.TWO_DIMENSIONAL_LITERAL);

		// Title
		Text tc = chart.getTitle().getLabel().getCaption();
		tc.setValue(getTitle());
		tc.getFont().setSize(11);
		tc.getFont().setName("Verdana");
		chart.getTitle().setVisible(isTitleVisible());
		
		// Legend
		chart.getLegend().setItemType(LegendItemType.CATEGORIES_LITERAL);
		chart.getLegend().setVisible(isLegendVisible());

		// X axis
		xAxis = chart.getPrimaryBaseAxes()[0];
		xAxis.getTitle().setVisible(false);
		xAxis.getLabel().setVisible(true);
		xAxis.getMajorGrid().setLineAttributes(LineAttributesImpl.create(ColorDefinitionImpl.create(0, 0, 0), LineStyle.DOTTED_LITERAL, 0));
		//xAxis.setType(AxisType.DATE_TIME_LITERAL);
		xAxis.setCategoryAxis(false);

		SeriesDefinition sdX = SeriesDefinitionImpl.create();
      xAxis.getSeriesDefinitions().add(sdX);
		
		// Y axis
		yAxis = chart.getPrimaryOrthogonalAxis(xAxis);
		yAxis.getTitle().setVisible(false);
		//yAxis.getScale().setMin(NumberDataElementImpl.create(0));
		yAxis.getMajorGrid().setLineAttributes(LineAttributesImpl.create(ColorDefinitionImpl.create(0, 0, 0), LineStyle.DOTTED_LITERAL, 0));
		yAxis.setType(useLogScale ? AxisType.LOGARITHMIC_LITERAL : AxisType.LINEAR_LITERAL);

		SeriesDefinition sdY = SeriesDefinitionImpl.create();
      yAxis.getSeriesDefinitions().add(sdY);

		for(GraphItem item : items)
		{
			DateTimeDataSet xds = DateTimeDataSetImpl.create(new long[] { 1000000000, 1000010000, 1000020000, 1000030000 } );
			
			Series xSeries = SeriesImpl.create();
			xSeries.setDataSet(xds);
			sdX.getSeries().add(xSeries);
			
			NumberDataSet yds = NumberDataSetImpl.create(new double[] { 7, 8, 4, 22 });
			
			LineSeries ySeries = (LineSeries)LineSeriesImpl.create();
			ySeries.setSeriesIdentifier(item.getDescription());
			ySeries.setDataSet(yds);
			ySeries.setLineAttributes(LineAttributesImpl.create(null, LineStyle.SOLID_LITERAL, 2));
			sdY.getSeries().add(ySeries);
		}
		
		return chart;
	}
	
	private String[] getElementNames()
	{
		String[] names = new String[items.size()];
		for(int i = 0; i < names.length; i++)
			names[i] = items.get(i).getDescription();
		return names;
	}
}
