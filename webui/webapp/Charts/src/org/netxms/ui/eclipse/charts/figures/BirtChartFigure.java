/**
 * 
 */
package org.netxms.ui.eclipse.charts.figures;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import org.eclipse.birt.chart.api.ChartEngine;
import org.eclipse.birt.chart.device.IDeviceRenderer;
import org.eclipse.birt.chart.exception.ChartException;
import org.eclipse.birt.chart.factory.GeneratedChartState;
import org.eclipse.birt.chart.factory.IGenerator;
import org.eclipse.birt.chart.model.Chart;
import org.eclipse.birt.chart.model.ChartWithAxes;
import org.eclipse.birt.chart.model.ChartWithoutAxes;
import org.eclipse.birt.chart.model.attribute.AxisType;
import org.eclipse.birt.chart.model.attribute.Bounds;
import org.eclipse.birt.chart.model.attribute.ChartDimension;
import org.eclipse.birt.chart.model.attribute.ColorDefinition;
import org.eclipse.birt.chart.model.attribute.LeaderLineStyle;
import org.eclipse.birt.chart.model.attribute.LegendItemType;
import org.eclipse.birt.chart.model.attribute.LineStyle;
import org.eclipse.birt.chart.model.attribute.Palette;
import org.eclipse.birt.chart.model.attribute.Position;
import org.eclipse.birt.chart.model.attribute.RiserType;
import org.eclipse.birt.chart.model.attribute.Text;
import org.eclipse.birt.chart.model.attribute.impl.BoundsImpl;
import org.eclipse.birt.chart.model.attribute.impl.ColorDefinitionImpl;
import org.eclipse.birt.chart.model.attribute.impl.LineAttributesImpl;
import org.eclipse.birt.chart.model.attribute.impl.PaletteImpl;
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
import org.eclipse.birt.core.framework.PlatformConfig;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.charts.api.DataComparisonChart;
import org.netxms.ui.eclipse.charts.widgets.internal.DataComparisonElement;
import org.netxms.ui.eclipse.tools.ColorCache;

/**
 * Figure to display BIRT chart
 */
public class BirtChartFigure extends GenericChartFigure implements DataComparisonChart
{
	private static final String CHART_FONT_NAME = "Verdana"; //$NON-NLS-1$
	private static final int CHART_FONT_SIZE_TITLE = 8;
	private static final int CHART_FONT_SIZE_LEGEND = 7;
	private static final int CHART_FONT_SIZE_AXIS = 7;
	
	private int chartType = BAR_CHART;
	private Chart chart = null;
	private boolean fullRepaint = true;
	private IDeviceRenderer deviceRenderer = null;
	private GeneratedChartState generatedChartState = null;
	private IGenerator generator = null;
	private Image imgChart = null;
	private ColorCache colors;
	private Set<String> errors = new HashSet<String>(0);
	private Image errorImage = null;
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
	public BirtChartFigure(int chartType, ColorCache colors)
	{
		super();
		
		this.chartType = chartType;
		if ((chartType == PIE_CHART) || (chartType == GAUGE_CHART))
			labelsVisible = true;

		this.colors = colors;
		
		try
		{
			PlatformConfig config = new PlatformConfig();
			config.setBIRTHome(""); //$NON-NLS-1$
			final ChartEngine chartEngine = ChartEngine.instance(config);
			deviceRenderer = chartEngine.getRenderer("dv.PNG"); //$NON-NLS-1$
			generator = chartEngine.getGenerator();
		}
		catch(ChartException e)
		{
			/* TODO: implement logging or error reporting */
			e.printStackTrace();
		}
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.figures.GenericChartFigure#dispose()
	 */
	@Override
	public void dispose()
	{
		super.dispose();
		if ((imgChart != null) && (!imgChart.isDisposed()))
			imgChart.dispose();
	}

	/**
	 * Force chart recreation (for example, after chart type change).
	 */
	protected void recreateChart()
	{
		chart = createChart();
		fullRepaint = true;
		repaint();
	}

	/**
	 * @return
	 */
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
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#initializationComplete()
	 */
	@Override
	public void initializationComplete()
	{
		chart = createChart();
		fullRepaint = true;
		repaint();
	}

	/**
	 * Create BIRT color definition object from preference string
	 *  
	 * @param name Preference name
	 * @return color definition object
	 */
	protected ColorDefinition getColorFromPreferences(final String name)
	{
		RGB rgb = PreferenceConverter.getColor(preferenceStore, name);
		return ColorDefinitionImpl.create(rgb.red, rgb.green, rgb.blue);
	}
	
	/**
	 * Get Position object from int constant
	 * @param value
	 * @return
	 */
	protected Position positionFromInt(int value)
	{
		switch(value)
		{
			case GraphSettings.POSITION_LEFT:
				return Position.LEFT_LITERAL;
			case GraphSettings.POSITION_RIGHT:
				return Position.RIGHT_LITERAL;
			case GraphSettings.POSITION_TOP:
				return Position.ABOVE_LITERAL;
			case GraphSettings.POSITION_BOTTOM:
				return Position.BELOW_LITERAL;
		}
		return Position.RIGHT_LITERAL;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
		if (chart == null)
			return;

		Rectangle clientArea = this.getClientArea();
		if ((clientArea.width == 0) || (clientArea.height == 0))
			return;
		
		if (fullRepaint)
		{
			// if resized
			// imgChart is factory created and should not be disposed
			imgChart = null;

			try
			{
				File tmpFile = File.createTempFile("birt_" + hashCode(), "_" + clientArea.width + "_" + clientArea.height);

				deviceRenderer.setProperty(IDeviceRenderer.FILE_IDENTIFIER, tmpFile);
				final Bounds bounds = BoundsImpl.create(0, 0, clientArea.width, clientArea.height);
				bounds.scale(72.0 / deviceRenderer.getDisplayServer().getDpiResolution());

				generatedChartState = generator.build(deviceRenderer.getDisplayServer(), chart, bounds, null, null, null);
				generator.render(deviceRenderer, generatedChartState);

				InputStream inputStream = null;
				try
				{
					inputStream = new FileInputStream(tmpFile);
					imgChart = new Image(Display.getCurrent(), inputStream);
					tmpFile.delete();
				}
				finally
				{
					if (inputStream != null)
					{
						inputStream.close();
					}
				}
			}
			catch(Exception e)
			{
				/* TODO: add logging and/or user notification */
				e.printStackTrace();
			}

			fullRepaint = false;
		}

		gc.drawImage(imgChart, clientArea.x, clientArea.y);
		paintErrorIndicator(gc);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#rebuild()
	 */
	@Override
	public void rebuild()
	{
		recreateChart();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTitle(java.lang.String)
	 */
	@Override
	public void setChartTitle(String title)
	{
		this.title = title;
		if (chart != null)
		{
			chart.getTitle().getLabel().getCaption().setValue(title);
			refresh();
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendVisible(boolean)
	 */
	@Override
	public void setLegendVisible(boolean visible)
	{
		legendVisible = visible;
		if (chart != null)
		{
			chart.getLegend().setVisible(visible);
			refresh();
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTitleVisible(boolean)
	 */
	@Override
	public void setTitleVisible(boolean visible)
	{
		titleVisible = visible;
		if (chart != null)
			chart.getTitle().setVisible(visible);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#set3DModeEnabled(boolean)
	 */
	@Override
	public void set3DModeEnabled(boolean enabled)
	{
		displayIn3D = enabled;
		if (chart != null)
			recreateChart();
	}
	
	/**
	 * Get palette usable by BIRT
	 * @return BIRT palette object
	 */
	protected Palette getBirtPalette()
	{
		Palette birtPalette = PaletteImpl.create(0, true);
		for(ChartColor c : palette)
			birtPalette.getEntries().add(ColorDefinitionImpl.create(c.red, c.green, c.blue, c.alpha));
		return birtPalette;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLogScaleEnabled(boolean)
	 */
	@Override
	public void setLogScaleEnabled(boolean enabled)
	{
		if (useLogScale != enabled)
		{
			useLogScale = enabled;
			if (chart != null)
				recreateChart();
		}
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.widgets.GenericChart#setTranslucent(boolean)
	 */
	@Override
	public void setTranslucent(boolean translucent)
	{
		super.setTranslucent(translucent);
		if (chart != null)
			recreateChart();
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#addError(java.lang.String)
	 */
	@Override
	public void addError(String message)
	{
		if (errors.add(message))
			repaint();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#clearErrors()
	 */
	@Override
	public void clearErrors()
	{
		if (errors.size() > 0)
		{
			errors.clear();
			repaint();
		}
	}
	
	/**
	 * Draw error indicator if needed
	 * 
	 * @param gc
	 */
	private void paintErrorIndicator(Graphics gc)
	{
		if (errors.size() == 0)
			return;

		if (errorImage == null)
			errorImage = Activator.getImageDescriptor("icons/chart_error.png").createImage(); //$NON-NLS-1$

		gc.setAlpha(127);
		gc.setBackgroundColor(colors.create(127, 127, 127));
		gc.fillRectangle(getClientArea());
		gc.setAlpha(255);
		gc.drawImage(errorImage, 10, 10);
		
		gc.setForegroundColor(colors.create(192, 0, 0));
		Iterator<String> it = errors.iterator();
		int y = 12;
		int h = 12; //gc.textExtent("X").y; //$NON-NLS-1$
		while(it.hasNext())
		{
			gc.drawText(it.next(), 40, y);
			y += h + 5;
		}
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
		if ((this.chartType != chartType) && (chart != null))
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
		fullRepaint = true;
		repaint();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataComparisionChart#setTransposed(boolean)
	 */
	@Override
	public void setTransposed(boolean transposed)
	{
		this.transposed = transposed;
		if (chart != null)
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
		if (chart != null)
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
		if (chart != null)
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
