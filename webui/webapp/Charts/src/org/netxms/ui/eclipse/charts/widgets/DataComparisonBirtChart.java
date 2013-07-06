/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
import org.eclipse.birt.chart.model.attribute.AxisType;
import org.eclipse.birt.chart.model.attribute.ChartDimension;
import org.eclipse.birt.chart.model.attribute.LeaderLineStyle;
import org.eclipse.birt.chart.model.attribute.LegendItemType;
import org.eclipse.birt.chart.model.attribute.LineStyle;
import org.eclipse.birt.chart.model.attribute.Position;
import org.eclipse.birt.chart.model.attribute.RiserType;
import org.eclipse.birt.chart.model.attribute.Text;
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
import org.eclipse.birt.chart.model.type.BarSeries;
import org.eclipse.birt.chart.model.type.PieSeries;
import org.eclipse.birt.chart.model.type.impl.BarSeriesImpl;
import org.eclipse.birt.chart.model.type.impl.PieSeriesImpl;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement;

/**
 * BIRT-based data comparison chart
 */
public class DataComparisonBirtChart extends GenericBirtChart implements DataComparisonChart
{
	private static final String CHART_FONT_NAME = "Verdana";
	private static final int CHART_FONT_SIZE_TITLE = 10;
	private static final int CHART_FONT_SIZE_LEGEND = 8;
	private static final int CHART_FONT_SIZE_AXIS = 8;
	
	private int chartType = BAR_CHART;
	private List<DataComparisonElement> parameters = new ArrayList<DataComparisonElement>(MAX_CHART_ITEMS);
	private Axis xAxis = null;
	private Axis yAxis = null;
	private Series valueSeries = null;
	private boolean transposed = false;
	private boolean labelsVisible = false;
	private double rotation = 0.0;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DataComparisonBirtChart(Composite parent, int style, int chartType)
	{
		super(parent, style);
		this.chartType = chartType;
		if ((chartType == PIE_CHART) || (chartType == DIAL_CHART))
			labelsVisible = true;
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
			case TUBE_CHART:
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
		chart.setTransposed(transposed);
		chart.getBlock().setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$
		chart.getPlot().setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$
		chart.getPlot().getClientArea().setBackground(getColorFromPreferences("Chart.Colors.PlotArea")); //$NON-NLS-1$

		// Title
		Text tc = chart.getTitle().getLabel().getCaption();
		tc.setValue(getChartTitle());
		tc.getFont().setName(CHART_FONT_NAME);
		tc.getFont().setSize(CHART_FONT_SIZE_TITLE);
		tc.getFont().setBold(true);
		chart.getTitle().setVisible(isTitleVisible());
		
		// Legend
		chart.getLegend().setItemType(LegendItemType.CATEGORIES_LITERAL);
		chart.getLegend().setVisible(isLegendVisible());
		chart.getLegend().setPosition(positionFromInt(legendPosition));
		chart.getLegend().setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$
		chart.getLegend().getText().getFont().setName(CHART_FONT_NAME);
		chart.getLegend().getText().getFont().setSize(CHART_FONT_SIZE_LEGEND);
		chart.getLegend().getText().getFont().setBold(false);
		
		// X axis
		xAxis = chart.getPrimaryBaseAxes()[0];
		xAxis.getTitle().setVisible(false);
		xAxis.getLabel().setVisible(false);
		
		// Y axis
		yAxis = chart.getPrimaryOrthogonalAxis(xAxis);
		yAxis.getTitle().setVisible(false);
		yAxis.getScale().setMin(NumberDataElementImpl.create(0));
		yAxis.getMajorGrid().setLineAttributes(LineAttributesImpl.create(getColorFromPreferences("Chart.Grid.Y.Color"), LineStyle.DOTTED_LITERAL, 0)); //$NON-NLS-1$
		yAxis.setType(useLogScale ? AxisType.LOGARITHMIC_LITERAL : AxisType.LINEAR_LITERAL);
		yAxis.getLabel().getCaption().getFont().setName(CHART_FONT_NAME);
		yAxis.getLabel().getCaption().getFont().setSize(CHART_FONT_SIZE_AXIS);
		
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
	 * Create chart without axes: pie, radar, etc.
	 * 
	 * @return
	 */
	private Chart createChartWithoutAxes()
	{
		ChartWithoutAxes chart = ChartWithoutAxesImpl.create();
		chart.setDimension(is3DModeEnabled() ? ChartDimension.TWO_DIMENSIONAL_WITH_DEPTH_LITERAL : ChartDimension.TWO_DIMENSIONAL_LITERAL);
		chart.getBlock().setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$
		chart.getPlot().setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$
		// For chart without axes, we wish to paint plot area with same background color as other chart parts
		chart.getPlot().getClientArea().setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$

		// Title
		Text tc = chart.getTitle().getLabel().getCaption();
		tc.setValue(getChartTitle());
		tc.getFont().setName(CHART_FONT_NAME);
		tc.getFont().setSize(CHART_FONT_SIZE_TITLE);
		tc.getFont().setBold(true);
		chart.getTitle().setVisible(isTitleVisible());
		
		// Legend
		chart.getLegend().setItemType(LegendItemType.CATEGORIES_LITERAL);
		chart.getLegend().setVisible(isLegendVisible());
		chart.getLegend().setPosition(positionFromInt(legendPosition));
		chart.getLegend().setBackground(getColorFromPreferences("Chart.Colors.Background")); //$NON-NLS-1$
		chart.getLegend().getText().getFont().setName(CHART_FONT_NAME);
		chart.getLegend().getText().getFont().setSize(CHART_FONT_SIZE_LEGEND);
		chart.getLegend().getText().getFont().setBold(false);
		
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
			case TUBE_CHART:
				BarSeries bs = (BarSeries)BarSeriesImpl.create();
				bs.setTranslucent(translucent);
				if (chartType == TUBE_CHART)
					bs.setRiser(RiserType.TUBE_LITERAL);
				bs.getLabel().setVisible(labelsVisible);
				return bs;
			case PIE_CHART:
				PieSeries ps = (PieSeries)PieSeriesImpl.create();
				ps.setRotation(rotation);
				if (is3DModeEnabled())
				{
					ps.setExplosion(3);
					ps.setRatio(0.4);
				}
				else
				{
					ps.setExplosion(0);
					ps.setRatio(1);
				}
				ps.setTranslucent(translucent);
				ps.getLabel().setVisible(labelsVisible);
				if (labelsVisible)
				{
					ps.setLabelPosition(Position.OUTSIDE_LITERAL);
					ps.setLeaderLineStyle(LeaderLineStyle.FIXED_LENGTH_LITERAL);
					ps.getLeaderLineAttributes().setVisible(true);
					ps.getLabel().getCaption().getFont().setName(CHART_FONT_NAME);
					ps.getLabel().getCaption().getFont().setSize(CHART_FONT_SIZE_AXIS);
				}
				return ps;
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
	public int addParameter(GraphItem dci, double value)
	{
		parameters.add(new DataComparisonElement(dci, value));
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
			parameters.get(index).setValue(value);
		}
		catch(IndexOutOfBoundsException e)
		{
		}

 		if (updateChart)
			refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisionChart#updateParameterThresholds(int, org.netxms.client.datacollection.Threshold[])
	 */
	@Override
	public void updateParameterThresholds(int index, Threshold[] thresholds)
	{
		try
		{
			parameters.get(index).setThresholds(thresholds);
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

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisionChart#setTransposed(boolean)
	 */
	@Override
	public void setTransposed(boolean transposed)
	{
		this.transposed = transposed;
		if (getChart() != null)
			recreateChart();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisionChart#isTransposed()
	 */
	@Override
	public boolean isTransposed()
	{
		return transposed;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#hasAxes()
	 */
	@Override
	public boolean hasAxes()
	{
		return chartType == BAR_CHART;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#setLabelsVisible(boolean)
	 */
	@Override
	public void setLabelsVisible(boolean visible)
	{
		labelsVisible = visible;
		if (getChart() != null)
			recreateChart();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#isLabelsVisible()
	 */
	@Override
	public boolean isLabelsVisible()
	{
		return labelsVisible;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#setRotation(double)
	 */
	@Override
	public void setRotation(double angle)
	{
		rotation = angle;
		if (getChart() != null)
			recreateChart();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisonChart#getRotation()
	 */
	@Override
	public double getRotation()
	{
		return rotation;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#isGridVisible()
	 */
	@Override
	public boolean isGridVisible()
	{
		// TODO Auto-generated method stub
		return false;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setGridVisible(boolean)
	 */
	@Override
	public void setGridVisible(boolean visible)
	{
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setBackgroundColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setPlotAreaColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setPlotAreaColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendColor(org.netxms.ui.eclipse.charts.api.ChartColor, org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setLegendColor(ChartColor foreground, ChartColor background)
	{
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setAxisColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setAxisColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setGridColor(org.netxms.ui.eclipse.charts.api.ChartColor)
	 */
	@Override
	public void setGridColor(ChartColor color)
	{
		// TODO Auto-generated method stub
		
	}
}
