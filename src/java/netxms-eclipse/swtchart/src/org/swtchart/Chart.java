/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import java.text.DecimalFormat;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.swtchart.internal.ChartLayout;
import org.swtchart.internal.ChartLayoutData;
import org.swtchart.internal.ChartTitle;
import org.swtchart.internal.Legend;
import org.swtchart.internal.PlotArea;
import org.swtchart.internal.Title;
import org.swtchart.internal.axis.AxisSet;

/**
 * A chart which are composed of title, legend, axes and plot area.
 */
public class Chart extends Composite implements Listener
{
	/** cached tick step on Y axis */
	protected double cachedTickStep = 0;

	/** the title */
	private Title title;

	/** the legend */
	private Legend legend;

	/** the set of axes */
	private AxisSet axisSet;

	/** the plot area */
	private PlotArea plotArea;

	/** the orientation of chart which can be horizontal or vertical */
	private int orientation;

	/** the state indicating if compressing series is enabled */
	private boolean compressEnabled;
	
	/**
	 * Constructor.
	 * 
	 * @param parent
	 *           the parent composite on which chart is placed
	 * @param style
	 *           the style of widget to construct
	 */
	public Chart(Composite parent, int style)
	{
		super(parent, style);

		orientation = SWT.HORIZONTAL;
		compressEnabled = true;

		parent.layout();

		setLayout(new ChartLayout());

		title = new ChartTitle(this, SWT.NONE);
		title.setLayoutData(new ChartLayoutData(SWT.DEFAULT, 100));
		legend = new Legend(this, SWT.NONE);
		legend.setLayoutData(new ChartLayoutData(200, SWT.DEFAULT));
		plotArea = new PlotArea(this, SWT.NONE);
		axisSet = new AxisSet(this);

		updateLayout();

		addListener(SWT.Resize, this);
	}

	/**
	 * Gets the chart title.
	 * 
	 * @return the chart title
	 */
	public ITitle getTitle()
	{
		return title;
	}

	/**
	 * Gets the legend.
	 * 
	 * @return the legend
	 */
	public ILegend getLegend()
	{
		return legend;
	}

	/**
	 * Gets the set of axes.
	 * 
	 * @return the set of axes
	 */
	public IAxisSet getAxisSet()
	{
		return axisSet;
	}

	/**
	 * Gets the plot area.
	 * 
	 * @return the plot area
	 */
	public Canvas getPlotArea()
	{
		return plotArea;
	}

	/**
	 * Gets the set of series.
	 * 
	 * @return the set of series
	 */
	public ISeriesSet getSeriesSet()
	{
		return plotArea.getSeriesSet();
	}

	/*
	 * @see Control#setBackground(Color)
	 */
	@Override
	public void setBackground(Color color)
	{
		super.setBackground(color);

		for(Control child : getChildren())
		{
			if (!(child instanceof PlotArea) && !(child instanceof Legend))
			{
				child.setBackground(color);
			}
		}
	}

	/**
	 * Gets the background color in plot area. This method is identical with
	 * <tt>getPlotArea().getBackground()</tt>.
	 * 
	 * @return the background color in plot area
	 */
	public Color getBackgroundInPlotArea()
	{
		return plotArea.getBackground();
	}

	/**
	 * Sets the background color in plot area.
	 * 
	 * @param color
	 *           the background color in plot area. If <tt>null</tt> is given,
	 *           default background color will be set.
	 * @exception IllegalArgumentException
	 *               if given color is disposed
	 */
	public void setBackgroundInPlotArea(Color color)
	{
		if (color != null && color.isDisposed())
		{
			SWT.error(SWT.ERROR_INVALID_ARGUMENT);
		}
		plotArea.setBackground(color);
	}

	/**
	 * Sets the state of chart orientation. The horizontal orientation means that
	 * X axis is horizontal as usual, while the vertical orientation means that Y
	 * axis is horizontal.
	 * 
	 * @param orientation
	 *           the orientation which can be SWT.HORIZONTAL or SWT.VERTICAL
	 */
	public void setOrientation(int orientation)
	{
		if (orientation == SWT.HORIZONTAL || orientation == SWT.VERTICAL)
		{
			this.orientation = orientation;
		}
		updateLayout();
	}

	/**
	 * Gets the state of chart orientation. The horizontal orientation means that
	 * X axis is horizontal as usual, while the vertical orientation means that Y
	 * axis is horizontal.
	 * 
	 * @return the orientation which can be SWT.HORIZONTAL or SWT.VERTICAL
	 */
	public int getOrientation()
	{
		return orientation;
	}

	/**
	 * Enables compressing series. By default, compressing series is enabled, and
	 * normally there should be no usecase to disable it. However, if you suspect
	 * that something is wrong in compressing series, you can disable it to
	 * isolate the issue.
	 * 
	 * @param enabled
	 *           true if enabling compressing series
	 */
	public void enableCompress(boolean enabled)
	{
		compressEnabled = enabled;
	}

	/**
	 * Gets the state indicating if compressing series is enabled.
	 * 
	 * @return true if compressing series is enabled
	 */
	public boolean isCompressEnabled()
	{
		return compressEnabled;
	}

	/*
	 * @see Listener#handleEvent(Event)
	 */
	public void handleEvent(Event event)
	{
		switch(event.type)
		{
			case SWT.Resize:
				updateLayout();
				redraw();
				break;
			default:
				break;
		}
	}

	/**
	 * Updates the layout of chart elements.
	 */
	public void updateLayout()
	{
		if (legend != null)
		{
			legend.updateLayoutData();
		}

		if (title != null)
		{
			title.updateLayoutData();
		}

		if (axisSet != null)
		{
			axisSet.updateLayoutData();
		}

		layout();

		if (axisSet != null)
		{
			axisSet.refresh();
		}
	}

	/*
	 * @see Control#update()
	 */
	@Override
	public void update()
	{
		super.update();
		for(Control child : getChildren())
		{
			child.update();
		}
	}

	/*
	 * @see Control#redraw()
	 */
	@Override
	public void redraw()
	{
		super.redraw();
		for(Control child : getChildren())
		{
			child.redraw();
		}
	}

	/**
	 * Saves to file with given format.
	 * 
	 * @param filename
	 *           the file name
	 * @param format
	 *           the format (SWT.IMAGE_*). The supported formats depend on OS.
	 */
	public void save(String filename, int format)
	{
		Point size = getSize();
		GC gc = new GC(this);
		Image image = new Image(Display.getDefault(), size.x, size.y);
		gc.copyArea(image, 0, 0);
		gc.dispose();

		ImageData data = image.getImageData();
		ImageLoader loader = new ImageLoader();
		loader.data = new ImageData[] { data };
		loader.save(filename, format);
		image.dispose();
	}
	
	/**
	 * Get rounded value for tick mark
	 * 
	 * @param value
	 * @param step
	 * @return
	 */
	public static String roundedDecimalValue(double value, double step)
	{
		double absValue = Math.abs(value);
		if (absValue >= 10000000000000L)
		{
			return Long.toString(Math.round(value / 1000000000000L)) + "T";
		}
		else if (absValue >= 1000000000000L)
		{
			return new DecimalFormat("0.0").format(value / 1000000000000L) + "T"; //$NON-NLS-1$
		}
		else if (absValue >= 10000000000L)
		{
			return Long.toString(Math.round(value / 1000000000)) + "G";
		}
		else if (absValue >= 1000000000)
		{
			return new DecimalFormat("0.0").format(value / 1000000000) + "G"; //$NON-NLS-1$
		}
		else if (absValue >= 10000000)
		{
			return Long.toString(Math.round(value / 1000000)) + "M";
		}
		else if (absValue >= 1000000)
		{
			return new DecimalFormat("0.0").format(value / 1000000) + "M"; //$NON-NLS-1$
		}
		else if (absValue >= 10000)
		{
			return Long.toString(Math.round(value / 1000)) + "K";
		}
		else if (absValue >= 1000)
		{
			return new DecimalFormat("0.0").format(value / 1000) + "K"; //$NON-NLS-1$
		}
		else if ((absValue >= 1) && (step >= 1))
		{
			return Long.toString(Math.round(value));
		}
		else if (absValue == 0)
		{
			return "0"; //$NON-NLS-1$
		}
		else
		{
			if (step < 0.00001)
				return Double.toString(value);
			if (step < 0.0001)
				return new DecimalFormat("0.00000").format(value); //$NON-NLS-1$
			if (step < 0.001)
				return new DecimalFormat("0.0000").format(value); //$NON-NLS-1$
			if (step < 0.01)
				return new DecimalFormat("0.000").format(value); //$NON-NLS-1$
			return new DecimalFormat("0.00").format(value); //$NON-NLS-1$
		}
	}

	/**
	 * @param cachedTickStep the cachedTickStep to set
	 */
	public void setCachedTickStep(double cachedTickStep)
	{
		this.cachedTickStep = cachedTickStep;
	}
}
