/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.series;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Event;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.IDisposeListener;
import org.swtchart.IErrorBar;
import org.swtchart.ISeries;
import org.swtchart.ISeriesLabel;
import org.swtchart.Range;
import org.swtchart.IAxis.Direction;
import org.swtchart.internal.axis.Axis;
import org.swtchart.internal.compress.ICompress;

/**
 * Series.
 */
abstract public class Series implements ISeries
{
	/** the default series type */
	protected static final SeriesType DEFAULT_SERIES_TYPE = SeriesType.LINE;

	/** the x series */
	protected double[] xSeries;

	/** the y series */
	protected double[] ySeries;

	/** the minimum value of x series */
	protected double minX;

	/** the maximum value of x series */
	protected double maxX;

	/** the minimum value of y series */
	protected double minY;

	/** the maximum value of y series */
	protected double maxY;

   /** the average value of y series */
   protected double avgY;

	/** the series id */
	protected String id;

	/** the series name */
	protected String name = null;

	/** the compressor */
	protected ICompress compressor;

	/** the x axis id */
	protected int xAxisId;

	/** the y axis id */
	protected int yAxisId;

	/** the visibility of series */
	protected boolean visible;

	/** the state indicating whether x series are monotone increasing */
	protected boolean isXMonotoneIncreasing;

	/** the series type */
	protected SeriesType type;

	/** the series label */
	protected SeriesLabel seriesLabel;

	/** the x error bar */
	protected ErrorBar xErrorBar;

	/** the y error bar */
	protected ErrorBar yErrorBar;

	/** the chart */
	protected Chart chart;

	/** the state indicating if the series is a stacked type */
	protected boolean stackEnabled;

	/** the stack series */
	protected double[] stackSeries;

	/** the state indicating if the type of X series is <tt>Date</tt> */
	private boolean isDateSeries;

	/** the list of dispose listeners */
	private List<IDisposeListener> listeners;

	/**
	 * Constructor.
	 * 
	 * @param chart
	 *           the chart
	 * @param id
	 *           the series id
	 */
	protected Series(Chart chart, String id)
	{
		super();

		this.chart = chart;
		this.id = id;
		xAxisId = 0;
		yAxisId = 0;
		visible = true;
		type = DEFAULT_SERIES_TYPE;
		stackEnabled = false;
		isXMonotoneIncreasing = true;
		seriesLabel = new SeriesLabel();
		xErrorBar = new ErrorBar();
		yErrorBar = new ErrorBar();
		listeners = new ArrayList<IDisposeListener>();
	}

	/*
	 * @see ISeries#getId()
	 */
	public String getId()
	{
		return id;
	}

	/*
	 * @see ISeries#setVisible(boolean)
	 */
	public void setVisible(boolean visible)
	{
		if (this.visible == visible)
		{
			return;
		}

		this.visible = visible;

		((SeriesSet)chart.getSeriesSet()).updateStackAndRiserData();
	}

	/*
	 * @see ISeries#isVisible()
	 */
	public boolean isVisible()
	{
		return visible;
	}

	/*
	 * @see ISeries#getType()
	 */
	public SeriesType getType()
	{
		return type;
	}

	/*
	 * @see ISeries#isStackEnabled()
	 */
	public boolean isStackEnabled()
	{
		return stackEnabled;
	}

	/* (non-Javadoc)
	 * @see org.swtchart.ISeries#enableStack(boolean, boolean)
	 */
	@Override
	public void enableStack(boolean enabled, boolean update)
	{
		if (enabled && minY < 0)
		{
			throw new IllegalStateException("Stacked series cannot contain minus values.");
		}

		if (stackEnabled == enabled)
		{
			return;
		}

		stackEnabled = enabled;

		if (update)
		   ((SeriesSet)chart.getSeriesSet()).updateStackAndRiserData();
	}

	/*
	 * @see ISeries#setXSeries(double[])
	 */
	public void setXSeries(double[] series)
	{

		if (series == null)
		{
			SWT.error(SWT.ERROR_NULL_ARGUMENT);
			return; // to suppress warning...
		}

		xSeries = new double[series.length];
		System.arraycopy(series, 0, xSeries, 0, series.length);
		isDateSeries = false;

		if (xSeries.length == 0)
		{
			return;
		}

		// find the min and max value of x series
		minX = xSeries[0];
		maxX = xSeries[0];
		for(int i = 1; i < xSeries.length; i++)
		{
			if (minX > xSeries[i])
			{
				minX = xSeries[i];
			}
			if (maxX < xSeries[i])
			{
				maxX = xSeries[i];
			}

			if (xSeries[i - 1] > xSeries[i])
			{
				isXMonotoneIncreasing = false;
			}
		}

		setCompressor();

		compressor.setXSeries(xSeries);
		if (ySeries != null)
		{
			compressor.setYSeries(ySeries);
		}

		if (minX <= 0)
		{
			IAxis axis = chart.getAxisSet().getXAxis(xAxisId);
			if (axis != null)
			{
				axis.enableLogScale(false);
			}
		}
	}

	/*
	 * @see ISeries#getXSeries()
	 */
	public double[] getXSeries()
	{
		if (xSeries == null)
		{
			return null;
		}

		double[] copiedSeries = new double[xSeries.length];
		System.arraycopy(xSeries, 0, copiedSeries, 0, xSeries.length);

		return copiedSeries;
	}

	/*
	 * @see ISeries#setYSeries(double[])
	 */
	public void setYSeries(double[] series)
	{
		if (series == null)
		{
			SWT.error(SWT.ERROR_NULL_ARGUMENT);
			return; // to suppress warning...
		}

		ySeries = new double[series.length];
		System.arraycopy(series, 0, ySeries, 0, series.length);

		if (ySeries.length == 0)
		{
			return;
		}

		// find the min, max, and average value of y series
		minY = ySeries[0];
		maxY = ySeries[0];
		double sum = ySeries[0];
		for(int i = 1; i < ySeries.length; i++)
		{
			if (minY > ySeries[i])
			{
				minY = ySeries[i];
			}
			if (maxY < ySeries[i])
			{
				maxY = ySeries[i];
			}
			sum += ySeries[i];
		}
		avgY = sum / ySeries.length;

		if (xSeries == null || xSeries.length != series.length)
		{
			xSeries = new double[series.length];
			for(int i = 0; i < series.length; i++)
			{
				xSeries[i] = i;
			}
			minX = xSeries[0];
			maxX = xSeries[xSeries.length - 1];
			isXMonotoneIncreasing = true;
		}

		setCompressor();

		compressor.setXSeries(xSeries);
		compressor.setYSeries(ySeries);

		if (minX < 0)
		{
			IAxis axis = chart.getAxisSet().getXAxis(xAxisId);
			if (axis != null)
			{
				axis.enableLogScale(false);
			}
		}
		if (minY < 0)
		{
			IAxis axis = chart.getAxisSet().getYAxis(yAxisId);
			if (axis != null)
			{
				axis.enableLogScale(false);
			}
			stackEnabled = false;
		}
	}

	/*
	 * @see ISeries#getYSeries()
	 */
	public double[] getYSeries()
	{
		if (ySeries == null)
		{
			return null;
		}

		double[] copiedSeries = new double[ySeries.length];
		System.arraycopy(ySeries, 0, copiedSeries, 0, ySeries.length);

		return copiedSeries;
	}

	/*
	 * @see ISeries#setXDateSeries(Date[])
	 */
	public void setXDateSeries(Date[] series)
	{
		if (series == null)
		{
			SWT.error(SWT.ERROR_NULL_ARGUMENT);
			return; // to suppress warning...
		}

		double[] xDateSeries = new double[series.length];
		for(int i = 0; i < series.length; i++)
		{
			xDateSeries[i] = series[i].getTime();
		}
		setXSeries(xDateSeries);
		isDateSeries = true;
	}

	/*
	 * @see ISeries#getXDateSeries()
	 */
	public Date[] getXDateSeries()
	{
		if (!isDateSeries)
		{
			return null;
		}

		Date[] series = new Date[xSeries.length];
		for(int i = 0; i < series.length; i++)
		{
			series[i] = new Date((long)xSeries[i]);
		}
		return series;
	}

	/**
	 * Gets the state indicating if date series is set.
	 * 
	 * @return true if date series is set
	 */
	public boolean isDateSeries()
	{
		return isDateSeries;
	}

	/**
	 * Gets the state indicating if the series is valid stack series.
	 * 
	 * @return true if the series is valid stack series
	 */
	public boolean isValidStackSeries()
	{
		return stackEnabled && stackSeries != null && stackSeries.length > 0
				&& !chart.getAxisSet().getYAxis(yAxisId).isLogScaleEnabled();
	}

	/**
	 * Gets the X range of series.
	 * 
	 * @return the X range of series
	 */
	public Range getXRange()
	{
		return new Range(minX, maxX);
	}

	/**
	 * Gets the adjusted range to show all series in screen. This range includes
	 * the size of plot like symbol or bar.
	 * 
	 * @param axis
	 *           the axis
	 * @param length
	 *           the axis length in pixels
	 * @return the adjusted range
	 */
	abstract public Range getAdjustedRange(Axis axis, int length);

	/**
	 * Gets the Y range of series.
	 * 
	 * @return the Y range of series
	 */
	public Range getYRange()
	{
		double min = minY;
		double max = maxY;
		if (isValidStackSeries())
		{
			for(int i = 0; i < stackSeries.length; i++)
			{
				if (max < stackSeries[i])
				{
					max = stackSeries[i];
				}
			}
		}
		return new Range(min, max);
	}

	/**
	 * Gets the compressor.
	 * 
	 * @return the compressor
	 */
	protected ICompress getCompressor()
	{
		return compressor;
	}

	/**
	 * Sets the compressor.
	 */
	abstract protected void setCompressor();

	/*
	 * @see ISeries#getXAxisId()
	 */
	public int getXAxisId()
	{
		return xAxisId;
	}

	/*
	 * @see ISeries#setXAxisId(int)
	 */
	public void setXAxisId(int id)
	{
		if (xAxisId == id)
		{
			return;
		}

		IAxis axis = chart.getAxisSet().getXAxis(xAxisId);

		if (minX <= 0 && axis != null && axis.isLogScaleEnabled())
		{
			chart.getAxisSet().getXAxis(xAxisId).enableLogScale(false);
		}

		xAxisId = id;

		((SeriesSet)chart.getSeriesSet()).updateStackAndRiserData();
	}

	/*
	 * @see ISeries#getYAxisId()
	 */
	public int getYAxisId()
	{
		return yAxisId;
	}

	/*
	 * @see ISeries#setYAxisId(int)
	 */
	public void setYAxisId(int id)
	{
		yAxisId = id;
	}

	/*
	 * @see ISeries#getLabel()
	 */
	public ISeriesLabel getLabel()
	{
		return seriesLabel;
	}

	/*
	 * @see ISeries#getXErrorBar()
	 */
	public IErrorBar getXErrorBar()
	{
		return xErrorBar;
	}

	/*
	 * @see ISeries#getYErrorBar()
	 */
	public IErrorBar getYErrorBar()
	{
		return yErrorBar;
	}

	/**
	 * Sets the stack series
	 * 
	 * @param stackSeries
	 *           The stack series
	 */
	protected void setStackSeries(double[] stackSeries)
	{
		this.stackSeries = stackSeries;
	}

	/*
	 * @see ISeries#getPixelCoordinates(int)
	 */
	public Point getPixelCoordinates(int index)
	{

		// get the horizontal and vertical axes
		IAxis hAxis;
		IAxis vAxis;
		if (chart.getOrientation() == SWT.HORIZONTAL)
		{
			hAxis = chart.getAxisSet().getXAxis(xAxisId);
			vAxis = chart.getAxisSet().getYAxis(yAxisId);
		}
		else if (chart.getOrientation() == SWT.VERTICAL)
		{
			hAxis = chart.getAxisSet().getYAxis(yAxisId);
			vAxis = chart.getAxisSet().getXAxis(xAxisId);
		}
		else
		{
			throw new IllegalStateException("unknown chart orientation"); //$NON-NLS-1$
		}

		// get the pixel coordinates
		return new Point(getPixelCoordinate(hAxis, index), getPixelCoordinate(vAxis, index));
	}

	/**
	 * Gets the pixel coordinates with given axis and series index.
	 * 
	 * @param axis
	 *           the axis
	 * @param index
	 *           the series index
	 * @return the pixel coordinates
	 */
	private int getPixelCoordinate(IAxis axis, int index)
	{

		// get the data coordinate
		double dataCoordinate;
		if (axis.getDirection() == Direction.X)
		{
			if (axis.isCategoryEnabled())
			{
				dataCoordinate = index;
			}
			else
			{
				if (index < 0 || xSeries.length <= index)
				{
					throw new IllegalArgumentException("Series index is out of range."); //$NON-NLS-1$
				}
				dataCoordinate = xSeries[index];
			}
		}
		else if (axis.getDirection() == Direction.Y)
		{
			if (isValidStackSeries())
			{
				if (index < 0 || stackSeries.length <= index)
				{
					throw new IllegalArgumentException("Series index is out of range."); //$NON-NLS-1$
				}
				dataCoordinate = stackSeries[index];
			}
			else
			{
				if (index < 0 || ySeries.length <= index)
				{
					throw new IllegalArgumentException("Series index is out of range."); //$NON-NLS-1$
				}
				dataCoordinate = ySeries[index];
			}
		}
		else
		{
			throw new IllegalStateException("unknown axis direction"); //$NON-NLS-1$
		}

		// get the pixel coordinate
		return axis.getPixelCoordinate(dataCoordinate);
	}

	/**
	 * Gets the range with given margin.
	 * 
	 * @param lowerPlotMargin
	 *           the lower margin in pixels
	 * @param upperPlotMargin
	 *           the upper margin in pixels
	 * @param length
	 *           the axis length in pixels
	 * @param axis
	 *           the axis
	 * @param range
	 *           the range
	 * @return the range with margin
	 */
	protected Range getRangeWithMargin(int lowerPlotMargin, int upperPlotMargin, int length, Axis axis, Range range)
	{
		if (length == 0)
		{
			return range;
		}

		int lowerPixelCoordinate = axis.getPixelCoordinate(range.lower, range.lower, range.upper) + lowerPlotMargin
				* (axis.isHorizontalAxis() ? -1 : 1);
		int upperPixelCoordinate = axis.getPixelCoordinate(range.upper, range.lower, range.upper) + upperPlotMargin
				* (axis.isHorizontalAxis() ? 1 : -1);

		double lower = axis.getDataCoordinate(lowerPixelCoordinate, range.lower, range.upper);
		double upper = axis.getDataCoordinate(upperPixelCoordinate, range.lower, range.upper);

		return new Range(lower, upper);
	}

	/**
	 * Disposes SWT resources.
	 */
	protected void dispose()
	{
		for(IDisposeListener listener : listeners)
		{
			listener.disposed(new Event());
		}
	}

	/*
	 * @see IAxis#addDisposeListener(IDisposeListener)
	 */
	public void addDisposeListener(IDisposeListener listener)
	{
		listeners.add(listener);
	}

	/**
	 * Draws series.
	 * 
	 * @param gc
	 *           the graphics context
	 * @param width
	 *           the width to draw series
	 * @param height
	 *           the height to draw series
	 */
	public void draw(GC gc, int width, int height)
	{

		if (!visible || width < 0 || height < 0 || xSeries == null || xSeries.length == 0 || ySeries == null || ySeries.length == 0)
		{
			return;
		}

		Axis xAxis = (Axis)chart.getAxisSet().getXAxis(getXAxisId());
		Axis yAxis = (Axis)chart.getAxisSet().getYAxis(getYAxisId());
		if (xAxis == null || yAxis == null)
		{
			return;
		}

		draw(gc, width, height, xAxis, yAxis);
	}

	/**
	 * Draws series.
	 * 
	 * @param gc
	 *           the graphics context
	 * @param width
	 *           the width to draw series
	 * @param height
	 *           the height to draw series
	 * @param xAxis
	 *           the x axis
	 * @param yAxis
	 *           the y axis
	 */
	abstract protected void draw(GC gc, int width, int height, Axis xAxis, Axis yAxis);

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.swtchart.ISeries#setName(java.lang.String)
	 */
	@Override
	public void setName(String name)
	{
		this.name = name;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.swtchart.ISeries#getName()
	 */
	@Override
	public String getName()
	{
		return (name != null) ? name : id;
	}

   /**
    * @return the minY
    */
   public double getMinY()
   {
      return minY;
   }

   /**
    * @return the maxY
    */
   public double getMaxY()
   {
      return maxY;
   }

   /**
    * @return the avgY
    */
   public double getAvgY()
   {
      return avgY;
   }
   
   /**
    * @return current Y value
    */
   public double getCurY()
   {
      return ((ySeries != null) && (ySeries.length > 0)) ? ySeries[0] : 0;
   }
}
