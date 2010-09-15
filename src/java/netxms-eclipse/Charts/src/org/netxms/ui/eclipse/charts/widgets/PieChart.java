package org.netxms.ui.eclipse.charts.widgets;

import org.eclipse.birt.chart.api.ChartEngine;
import org.eclipse.birt.chart.device.IDeviceRenderer;
import org.eclipse.birt.chart.exception.ChartException;
import org.eclipse.birt.chart.factory.GeneratedChartState;
import org.eclipse.birt.chart.factory.IDataRowExpressionEvaluator;
import org.eclipse.birt.chart.factory.IGenerator;
import org.eclipse.birt.chart.factory.RunTimeContext;
import org.eclipse.birt.chart.model.ChartWithoutAxes;
import org.eclipse.birt.chart.model.attribute.Bounds;
import org.eclipse.birt.chart.model.attribute.ChartDimension;
import org.eclipse.birt.chart.model.attribute.impl.BoundsImpl;
import org.eclipse.birt.chart.model.component.Series;
import org.eclipse.birt.chart.model.component.impl.SeriesImpl;
import org.eclipse.birt.chart.model.data.BaseSampleData;
import org.eclipse.birt.chart.model.data.DataFactory;
import org.eclipse.birt.chart.model.data.OrthogonalSampleData;
import org.eclipse.birt.chart.model.data.Query;
import org.eclipse.birt.chart.model.data.SampleData;
import org.eclipse.birt.chart.model.data.SeriesDefinition;
import org.eclipse.birt.chart.model.data.impl.QueryImpl;
import org.eclipse.birt.chart.model.data.impl.SeriesDefinitionImpl;
import org.eclipse.birt.chart.model.impl.ChartWithoutAxesImpl;
import org.eclipse.birt.chart.model.layout.Legend;
import org.eclipse.birt.chart.model.type.PieSeries;
import org.eclipse.birt.chart.model.type.impl.PieSeriesImpl;
import org.eclipse.birt.core.framework.PlatformConfig;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.charts.Activator;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.IAxisSet;
import org.swtchart.IAxisTick;
import org.swtchart.IBarSeries;
import org.swtchart.ILegend;
import org.swtchart.ISeriesSet;
import org.swtchart.ISeries.SeriesType;

public class PieChart extends Chart implements ControlListener
{
	private IPreferenceStore preferenceStore;
	private boolean showToolTips;

	private Bounds bounds;
	private ChartWithoutAxes birtChart = null;
	private IDeviceRenderer deviceRenderer = null;
	private boolean fullRepaint = true;
	private GC gcImage;
	private GeneratedChartState generatedChartState = null;
	private Image imgChart;
	private IGenerator generator;

	public PieChart(Composite parent, int style)
	{
		super(parent, style);

		preferenceStore = Activator.getDefault().getPreferenceStore();
		showToolTips = preferenceStore.getBoolean("Chart.ShowToolTips");
		setBackground(getColorFromPreferences("Chart.Colors.Background"));

		setupSwtChart();
		setupBirtChart();

		final Object[][] data = new Object[][] {
				{ "Label 1", new Double(1) }, { "Label 2", new Double(4) }, { "Лейбел 3", new Double(7) } };
		bindData(new IDataRowExpressionEvaluator()
		{
			int idx = 0;

			public void close()
			{
			}

			public Object evaluate(String expression)
			{
				if ("Label".equals(expression))
				{
					return data[idx][0];
				}
				else if ("Value".equals(expression))
				{
					return data[idx][1];
				}
				return null;
			}

			public Object evaluateGlobal(String expression)
			{
				return evaluate(expression);
			}

			public boolean first()
			{
				idx = 0;
				return true;
			}

			public boolean next()
			{
				idx++;
				return (idx < data.length);
			}
		});

	}

	private void bindData(IDataRowExpressionEvaluator evaluator)
	{
		try
		{
			generator.bindData(evaluator, birtChart, new RunTimeContext());
		}
		catch(ChartException e)
		{
			e.printStackTrace();

		}
	}

	private void setupBirtChart()
	{
		initBirtPlatform();
		createBirtPieChart();

		getPlotArea().addPaintListener(new PaintListener()
		{

			@Override
			public void paintControl(PaintEvent e)
			{
				paintBirtControl(e);
			}
		});

		getPlotArea().addControlListener(this);
		//addControlListener(this);
	}

	private void createBirtPieChart()
	{
		birtChart = ChartWithoutAxesImpl.create();
		birtChart.setDimension(ChartDimension.TWO_DIMENSIONAL_WITH_DEPTH_LITERAL);
		birtChart.setType("Pie Chart");

		birtChart.setSeriesThickness(10);

		//chart.getTitle().getLabel().getCaption().setValue("Test Title");
		birtChart.getTitle().setVisible(false);

		Legend legend = birtChart.getLegend();
		legend.getOutline().setVisible(false);
		legend.setVisible(false);

		SampleData sampleData = DataFactory.eINSTANCE.createSampleData();
		BaseSampleData baseSampleData = DataFactory.eINSTANCE.createBaseSampleData();
		baseSampleData.setDataSetRepresentation("");
		sampleData.getBaseSampleData().add(baseSampleData);

		OrthogonalSampleData orthogonalSampleData = DataFactory.eINSTANCE.createOrthogonalSampleData();
		orthogonalSampleData.setDataSetRepresentation("");
		orthogonalSampleData.setSeriesDefinitionIndex(0);
		sampleData.getOrthogonalSampleData().add(orthogonalSampleData);

		birtChart.setSampleData(sampleData);

		Query queryLabel = QueryImpl.create("Label");

		Series category = SeriesImpl.create();
		category.getDataDefinition().add(queryLabel);

		SeriesDefinition seriesDefinitionLabel = SeriesDefinitionImpl.create();
		birtChart.getSeriesDefinitions().add(seriesDefinitionLabel);
		seriesDefinitionLabel.getSeriesPalette().shift(0);
		seriesDefinitionLabel.getSeries().add(category);

		Query queryValue = QueryImpl.create("Value");
		PieSeries pieSeries = (PieSeries) PieSeriesImpl.create();
		pieSeries.getDataDefinition().add(queryValue);
		//pieSeries.setSeriesIdentifier("Sample Values");
		pieSeries.setExplosion(0);

		SeriesDefinition seriesDefinitionValue = SeriesDefinitionImpl.create();
		seriesDefinitionLabel.getSeriesDefinitions().add(seriesDefinitionValue);
		seriesDefinitionValue.getSeries().add(pieSeries);
	}

	private void initBirtPlatform()
	{
		try
		{
			PlatformConfig config = new PlatformConfig();
			config.setBIRTHome("");
			final ChartEngine chartEngine = ChartEngine.instance(config);
			deviceRenderer = chartEngine.getRenderer("dv.SWT");
			generator = chartEngine.getGenerator();
		}
		catch(ChartException pex)
		{
			pex.printStackTrace();
		}
	}

	private void setupSwtChart()
	{
		getTitle().setVisible(false);

		// Setup legend
		ILegend legend = getLegend();
		legend.setPosition(SWT.RIGHT);
		legend.setVisible(true);
		
		// Setup X and Y axis
		IAxisSet axisSet = getAxisSet();
		final IAxis xAxis = axisSet.getXAxis(0);
		xAxis.getTitle().setVisible(false);
		IAxisTick xTick = xAxis.getTick();
		xTick.setForeground(getColorFromPreferences("Chart.Axis.X.Color"));
		xTick.setVisible(false);

		final IAxis yAxis = axisSet.getYAxis(0);
		yAxis.getTitle().setVisible(false);
		final IAxisTick yTick = yAxis.getTick();
		yTick.setForeground(getColorFromPreferences("Chart.Axis.Y.Color"));
		yTick.setVisible(false);

		// Setup plot area
		setBackgroundInPlotArea(getColorFromPreferences("Chart.Colors.PlotArea"));
	}

	/**
	 * Create color object from preference string
	 * 
	 * @param name
	 *           Preference name
	 * @return Color object
	 */
	private Color getColorFromPreferences(final String name)
	{
		return new Color(getDisplay(), PreferenceConverter.getColor(preferenceStore, name));
	}

	/**
	 * Add series to chart
	 * 
	 * @param description
	 *           Description
	 * @param xSeries
	 *           X axis data
	 * @param ySeries
	 *           Y axis data
	 */
	public IBarSeries addPieSeries(int index, String description, double value)
	{
		ISeriesSet seriesSet = getSeriesSet();
		IBarSeries series = (IBarSeries) seriesSet.createSeries(SeriesType.BAR, description);

		series.setBarColor(getColorFromPreferences("Chart.Colors.Data." + index));
		series.setBarPadding(25);

		series.setYSeries(new double[] { value });

		return series;
	}

	/**
	 * Enable categories on bar chart
	 * 
	 * @param categories
	 */
	public void setCategories(String[] categories)
	{
		IAxis axis = this.getAxisSet().getXAxis(0);
		axis.enableCategory(true);
		axis.setCategorySeries(categories);
	}

	@Override
	public void controlMoved(ControlEvent e)
	{
		fullRepaint = true;
	}

	@Override
	public void controlResized(ControlEvent e)
	{
		fullRepaint = true;
	}

	@Override
	public void dispose()
	{
		if ((imgChart != null) && (!imgChart.isDisposed()))
		{
			imgChart.dispose();
		}
		if ((gcImage != null) && (!gcImage.isDisposed()))
		{
			gcImage.dispose();
		}
		super.dispose();
	}

	private void paintBirtControl(PaintEvent event)
	{
		Rectangle clientArea = getPlotArea().getClientArea();
		if (fullRepaint)
		{
			//if resized
			if ((imgChart != null) && (!imgChart.isDisposed()))
				imgChart.dispose();
			if ((gcImage != null) && (!gcImage.isDisposed()))
				gcImage.dispose();

			imgChart = new Image(this.getDisplay(), clientArea);
			gcImage = new GC(imgChart);
			deviceRenderer.setProperty(IDeviceRenderer.GRAPHICS_CONTEXT, gcImage);
			bounds = BoundsImpl.create(0, 0, clientArea.width, clientArea.height);
			bounds.scale(72d / deviceRenderer.getDisplayServer().getDpiResolution());

			fullRepaint = false;
		}

		try
		{
			generatedChartState = generator.build(deviceRenderer.getDisplayServer(), birtChart, bounds, null, null, null);
			generator.render(deviceRenderer, generatedChartState);
			GC gc = event.gc;
			gc.drawImage(imgChart, clientArea.x, clientArea.y);
		}
		catch(ChartException e)
		{
			e.printStackTrace();
		}
	}
}
