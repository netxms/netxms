package org.netxms.ui.eclipse.charts.widgets;

import org.eclipse.birt.chart.api.ChartEngine;
import org.eclipse.birt.chart.device.IDeviceRenderer;
import org.eclipse.birt.chart.exception.ChartException;
import org.eclipse.birt.chart.factory.GeneratedChartState;
import org.eclipse.birt.chart.factory.IDataRowExpressionEvaluator;
import org.eclipse.birt.chart.factory.IGenerator;
import org.eclipse.birt.chart.factory.RunTimeContext;
import org.eclipse.birt.chart.model.Chart;
import org.eclipse.birt.chart.model.attribute.Bounds;
import org.eclipse.birt.chart.model.attribute.impl.BoundsImpl;
import org.eclipse.birt.core.framework.PlatformConfig;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;

public abstract class GenericBirtChart extends Composite implements PaintListener, ControlListener
{
	protected abstract Chart createChart();

	private boolean fullRepaint = true;

	private Bounds bounds;
	private Chart chart = null;

	private IDeviceRenderer deviceRenderer = null;

	private GC gcImage;

	private GeneratedChartState generatedChartState = null;

	private IGenerator generator;

	public Chart getChart()
	{
		return chart;
	}

	// Live Date Set
	private Image imgChart;

	public GenericBirtChart(Composite parent, int style)
	{
		super(parent, style);
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
		chart = createChart();
	}

	public void bindData(IDataRowExpressionEvaluator evaluator)
	{
		try
		{
			generator.bindData(evaluator, chart, new RunTimeContext());

		}
		catch(ChartException e)
		{
			e.printStackTrace();

		}
	}

	public void setTitle(String title)
	{
		chart.getTitle().getLabel().getCaption().setValue(title);
	}

	@Override
	public void controlMoved(ControlEvent arg0)
	{
		fullRepaint = true;
	}

	@Override
	public void controlResized(ControlEvent arg0)
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

	public final void paintControl(PaintEvent event)
	{
		Rectangle clientArea = this.getClientArea();
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
			generatedChartState = generator.build(deviceRenderer.getDisplayServer(), chart, bounds, null, null, null);
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