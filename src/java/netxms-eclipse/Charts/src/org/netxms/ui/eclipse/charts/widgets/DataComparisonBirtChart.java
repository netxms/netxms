/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.charts.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.birt.chart.model.Chart;
import org.eclipse.birt.chart.model.ChartWithAxes;
import org.eclipse.birt.chart.model.ChartWithoutAxes;
import org.eclipse.birt.chart.model.attribute.ChartDimension;
import org.eclipse.birt.chart.model.attribute.LegendItemType;
import org.eclipse.birt.chart.model.attribute.LineStyle;
import org.eclipse.birt.chart.model.attribute.Position;
import org.eclipse.birt.chart.model.attribute.Text;
import org.eclipse.birt.chart.model.attribute.impl.ColorDefinitionImpl;
import org.eclipse.birt.chart.model.attribute.impl.LineAttributesImpl;
import org.eclipse.birt.chart.model.component.Axis;
import org.eclipse.birt.chart.model.component.Series;
import org.eclipse.birt.chart.model.component.impl.SeriesImpl;
import org.eclipse.birt.chart.model.data.NumberDataSet;
import org.eclipse.birt.chart.model.data.SeriesDefinition;
import org.eclipse.birt.chart.model.data.TextDataSet;
import org.eclipse.birt.chart.model.data.impl.NumberDataElementImpl;
import org.eclipse.birt.chart.model.data.impl.NumberDataSetImpl;
import org.eclipse.birt.chart.model.data.impl.SeriesDefinitionImpl;
import org.eclipse.birt.chart.model.data.impl.TextDataSetImpl;
import org.eclipse.birt.chart.model.impl.ChartWithAxesImpl;
import org.eclipse.birt.chart.model.impl.ChartWithoutAxesImpl;
import org.eclipse.birt.chart.model.type.PieSeries;
import org.eclipse.birt.chart.model.type.impl.BarSeriesImpl;
import org.eclipse.birt.chart.model.type.impl.BubbleSeriesImpl;
import org.eclipse.birt.chart.model.type.impl.PieSeriesImpl;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.charts.api.DataComparisionChart;
import org.netxms.ui.eclipse.charts.widgets.internal.DataComparisionElement;

/**
 * BIRT-based data comparision chart
 *
 */
public class DataComparisonBirtChart extends GenericBirtChart implements DataComparisionChart
{
	private int chartType = BAR_CHART;
	private List<DataComparisionElement> parameters = new ArrayList<DataComparisionElement>(MAX_CHART_ITEMS);
	private Axis xAxis = null;
	private Axis yAxis = null;
	private Series valueSeries = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DataComparisonBirtChart(Composite parent, int style, int chartType)
	{
		super(parent, style);
		this.chartType = chartType;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.widgets.GenericBirtChart#createChart()
	 */
	@Override
	protected Chart createChart()
	{
		Chart chart;
		
		switch(chartType)
		{
			case BAR_CHART:
			case BUBBLE_CHART:
				chart = createChartWithAxes();
				break;
			case PIE_CHART:
				chart = createChartWithoutAxes();
				break;
			default:
				chart = ChartWithoutAxesImpl.create();	// Create empty chart
				chart.getTitle().setVisible(false);
				break;
		}
		return chart;
	}
	
	/**
	 * Create chart with axes: bar, tube, bubble, etc.
	 * 
	 * @return
	 */
	private Chart createChartWithAxes()
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
		
		// Y axis
		yAxis = chart.getPrimaryOrthogonalAxis(xAxis);
		yAxis.getTitle().setVisible(false);
		yAxis.getScale().setMin(NumberDataElementImpl.create(0));
		yAxis.getMajorGrid().setLineAttributes(LineAttributesImpl.create(ColorDefinitionImpl.create(0, 0, 0), LineStyle.DOTTED_LITERAL, 0));
		
		// Categories
		TextDataSet categoryValues = TextDataSetImpl.create(getElementNames());
      Series seCategory = SeriesImpl.create();
      seCategory.setDataSet(categoryValues);
      SeriesDefinition sdX = SeriesDefinitionImpl.create();
      sdX.setSeriesPalette(getBirtPalette());
      xAxis.getSeriesDefinitions().add(sdX);
      sdX.getSeries().add(seCategory);
      
      // Values
      NumberDataSet valuesDataSet = NumberDataSetImpl.create(getElementValues());
      valueSeries = createSeriesImplementation();
      valueSeries.setDataSet(valuesDataSet);
      SeriesDefinition sdY = SeriesDefinitionImpl.create();
      yAxis.getSeriesDefinitions().add(sdY);
      sdY.getSeries().add(valueSeries);
		
		return chart;
	}
	
	/**
	 * Create chart without axes: pie, dial, etc.
	 * 
	 * @return
	 */
	private Chart createChartWithoutAxes()
	{
		ChartWithoutAxes chart = ChartWithoutAxesImpl.create();
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
		
		// Categories
      SeriesDefinition sdCategory = SeriesDefinitionImpl.create();
      sdCategory.setSeriesPalette(getBirtPalette());
      Series seCategory = SeriesImpl.create();
      seCategory.setDataSet(TextDataSetImpl.create(getElementNames()));
      sdCategory.getSeries().add(seCategory);
      chart.getSeriesDefinitions().add(sdCategory);
      
      // Values
      SeriesDefinition sdValues = SeriesDefinitionImpl.create();
      NumberDataSet valuesDataSet = NumberDataSetImpl.create(getElementValues());
      valueSeries = createSeriesImplementation();
      valueSeries.setDataSet(valuesDataSet);
      sdValues.getSeries().add(valueSeries);
      sdCategory.getSeriesDefinitions().add(sdValues);
		
		return chart;
	}
	
	/**
	 * Create series implementation for selected chart type
	 * @return
	 */
	private Series createSeriesImplementation()
	{
		switch(chartType)
		{
			case BAR_CHART:
				return BarSeriesImpl.create();
			case BUBBLE_CHART:
				return BubbleSeriesImpl.create();
			case PIE_CHART:
				PieSeries series = (PieSeries)PieSeriesImpl.create();
				series.setExplosion(5);
				series.setRatio(0.4);
				series.setTranslucent(true);
				series.setLabelPosition(Position.INSIDE_LITERAL);
				series.getLeaderLineAttributes().setVisible(false);
				series.getLeaderLineAttributes().setThickness(3);
				return series;
			default:
				return null;
		}
	}
	
	/**
	 * Get names of all chart elements
	 * @return array of elements' names
	 */
	private String[] getElementNames()
	{
		String[] names = new String[parameters.size()];
		for(int i = 0; i < names.length; i++)
			names[i] = parameters.get(i).getName();
		return names;
	}

	/**
	 * Get values of all chart elements
	 * @return array of elements' values
	 */
	private double[] getElementValues()
	{
		double[] values = new double[parameters.size()];
		for(int i = 0; i < values.length; i++)
			values[i] = parameters.get(i).getValue();
		return values;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisionChart#getChartType()
	 */
	@Override
	public int getChartType()
	{
		return chartType;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisionChart#setChartType(int)
	 */
	@Override
	public void setChartType(int chartType)
	{
		if ((this.chartType != chartType) && (getChart() != null))
		{
			this.chartType = chartType;
			recreateChart();
		}
		else
		{
			this.chartType = chartType;
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisionChart#addParameter(java.lang.String, double)
	 */
	@Override
	public int addParameter(String name, double value)
	{
		parameters.add(new DataComparisionElement(name, value));
		return parameters.size() - 1;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisionChart#updateParameter(int, double)
	 */
	@Override
	public void updateParameter(int index, double value, boolean updateChart)
	{
		try
		{
			DataComparisionElement e = parameters.get(index);
			e.setValue(value);
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
		if (valueSeries == null)
			return;

		valueSeries.setDataSet(NumberDataSetImpl.create(getElementValues()));
		super.refresh();
	}
}
