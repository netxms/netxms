/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.series;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.widgets.Display;
import org.swtchart.Chart;
import org.swtchart.ILineSeries;
import org.swtchart.LineStyle;
import org.swtchart.Range;
import org.swtchart.IAxis.Direction;
import org.swtchart.internal.Util;
import org.swtchart.internal.axis.Axis;
import org.swtchart.internal.compress.CompressLineSeries;
import org.swtchart.internal.compress.CompressScatterSeries;

/**
 * Line series.
 */
public class LineSeries extends Series implements ILineSeries
{

	/** the symbol size in pixel */
	private int symbolSize;

	/** the symbol color */
	private Color symbolColor;

	/** the symbol colors */
	private Color[] symbolColors;

	/** the symbol type */
	private PlotSymbolType symbolType;

	/** the line style */
	private LineStyle lineStyle;

	/** the line color */
	private Color lineColor;

	/** the line width */
	private int lineWidth;

	/** the state indicating if area chart is enabled */
	private boolean areaEnabled;

	/** the state indicating if step chart is enabled */
	private boolean stepEnabled;

	/** the anti-aliasing value for drawing line */
	private int antialias;

	/** the alpha value to draw area */
	private static final int ALPHA = 50;

	/** the default line style */
	private static final LineStyle DEFAULT_LINE_STYLE = LineStyle.SOLID;

	/** the default line width */
	private static final int DEFAULT_LINE_WIDTH = 1;

	/** the default line color */
	private static final int DEFAULT_LINE_COLOR = SWT.COLOR_BLUE;

	/** the default symbol color */
	private static final int DEFAULT_SYMBOL_COLOR = SWT.COLOR_DARK_GRAY;

	/** the default symbol size */
	private static final int DEFAULT_SIZE = 4;

	/** the default symbol type */
	private static final PlotSymbolType DEFAULT_SYMBOL_TYPE = PlotSymbolType.CIRCLE;

	/** the default anti-aliasing value */
	private static final int DEFAULT_ANTIALIAS = SWT.DEFAULT;

	/** the margin in pixels attached at the minimum/maximum plot */
	//private static final int MARGIN_AT_MIN_MAX_PLOT = 6;

	/**
	 * Constructor.
	 * 
	 * @param chart
	 *           the chart
	 * @param id
	 *           the series id
	 */
	protected LineSeries(Chart chart, String id)
	{
		super(chart, id);

		symbolSize = 4;
		symbolColor = Display.getDefault().getSystemColor(DEFAULT_SYMBOL_COLOR);
		symbolType = DEFAULT_SYMBOL_TYPE;

		lineStyle = DEFAULT_LINE_STYLE;
		lineColor = Display.getDefault().getSystemColor(DEFAULT_LINE_COLOR);

		areaEnabled = false;

		antialias = DEFAULT_ANTIALIAS;
		lineWidth = DEFAULT_LINE_WIDTH;

		compressor = new CompressLineSeries();
	}

	/*
	 * @see ILineSeries#getLineStyle()
	 */
	public LineStyle getLineStyle()
	{
		return lineStyle;
	}

	/*
	 * @see ILineSeries#setLineStyle(LineStyle)
	 */
	public void setLineStyle(LineStyle style)
	{
		if (style == null)
		{
			this.lineStyle = DEFAULT_LINE_STYLE;
			return;
		}

		this.lineStyle = style;
		if (compressor instanceof CompressScatterSeries)
		{
			((CompressScatterSeries)compressor).setLineVisible(style != LineStyle.NONE);
		}
	}

	/*
	 * @see ILineSeries#getLineColor()
	 */
	public Color getLineColor()
	{
		if (lineColor.isDisposed())
		{
			lineColor = Display.getDefault().getSystemColor(DEFAULT_LINE_COLOR);
		}
		return lineColor;
	}

	/*
	 * @see ILineSeries#setLineColor(Color)
	 */
	public void setLineColor(Color color)
	{
		if (color != null && color.isDisposed())
		{
			SWT.error(SWT.ERROR_INVALID_ARGUMENT);
		}

		if (color == null)
		{
			this.lineColor = Display.getDefault().getSystemColor(DEFAULT_LINE_COLOR);
		}
		else
		{
			this.lineColor = color;
		}
	}

	/*
	 * @see ILineSeries#getLineWidth()
	 */
	public int getLineWidth()
	{
		return lineWidth;
	}

	/*
	 * @see ILineSeries#setLineWidth(int)
	 */
	public void setLineWidth(int width)
	{
		if (width <= 0)
		{
			this.lineWidth = DEFAULT_LINE_WIDTH;
		}
		else
		{
			this.lineWidth = width;
		}
	}

	/*
	 * @see ILineSeries#getSymbolType()
	 */
	public PlotSymbolType getSymbolType()
	{
		return symbolType;
	}

	/*
	 * @see ILineSeries#setSymbolType(PlotSymbolType)
	 */
	public void setSymbolType(PlotSymbolType type)
	{
		if (type == null)
		{
			this.symbolType = DEFAULT_SYMBOL_TYPE;
		}
		else
		{
			this.symbolType = type;
		}
	}

	/*
	 * @see ILineSeries#getSymbolSize()
	 */
	public int getSymbolSize()
	{
		return symbolSize;
	}

	/*
	 * @see ILineSeries#setSymbolSize(int)
	 */
	public void setSymbolSize(int size)
	{
		if (size <= 0)
		{
			this.symbolSize = DEFAULT_SIZE;
		}
		else
		{
			this.symbolSize = size;
		}
	}

	/*
	 * @see ILineSeries#getSymbolColor()
	 */
	public Color getSymbolColor()
	{
		return symbolColor;
	}

	/*
	 * @see ILineSeries#setSymbolColor(Color)
	 */
	public void setSymbolColor(Color color)
	{
		if (color != null && color.isDisposed())
		{
			SWT.error(SWT.ERROR_INVALID_ARGUMENT);
		}

		if (color == null)
		{
			this.symbolColor = Display.getDefault().getSystemColor(DEFAULT_SYMBOL_COLOR);
		}
		else
		{
			this.symbolColor = color;
		}
	}

	/*
	 * @see ILineSeries#getSymbolColors()
	 */
	public Color[] getSymbolColors()
	{
		if (symbolColors == null)
		{
			return null;
		}

		Color[] copiedSymbolColors = new Color[symbolColors.length];
		System.arraycopy(symbolColors, 0, copiedSymbolColors, 0, symbolColors.length);

		return copiedSymbolColors;
	}

	/*
	 * @see ILineSeries#setSymbolColors(Color [])
	 */
	public void setSymbolColors(Color[] colors)
	{
		if (colors == null)
		{
			symbolColors = null;
			return;
		}

		for(Color color : colors)
		{
			if (color.isDisposed())
			{
				SWT.error(SWT.ERROR_INVALID_ARGUMENT);
			}
		}

		symbolColors = new Color[colors.length];
		System.arraycopy(colors, 0, symbolColors, 0, colors.length);
	}

	/*
	 * @see Series#setCompressor()
	 */
	@Override
	protected void setCompressor()
	{
		if (isXMonotoneIncreasing)
		{
			compressor = new CompressLineSeries();
		}
		else
		{
			compressor = new CompressScatterSeries();
			((CompressScatterSeries)compressor).setLineVisible(getLineStyle() != LineStyle.NONE);
		}
	}

	/*
	 * @see ILineSeries#enableArea(boolean)
	 */
	public void enableArea(boolean enabled)
	{
		areaEnabled = enabled;
	}

	/*
	 * @see ILineSeries#isAreaEnabled()
	 */
	public boolean isAreaEnabled()
	{
		return areaEnabled;
	}

	/*
	 * @see ILineSeries#enableStep(boolean)
	 */
	public void enableStep(boolean enabled)
	{
		stepEnabled = enabled;
	}

	/*
	 * @see ILineSeries#isStepEnabled()
	 */
	public boolean isStepEnabled()
	{
		return stepEnabled;
	}

	/*
	 * @see Series#getAdjustedRange(Axis, int)
	 */
	@Override
	public Range getAdjustedRange(Axis axis, int length)
	{

		Range range;
		if (axis.getDirection() == Direction.X)
		{
			range = getXRange();
		}
		else
		{
			range = getYRange();
		}
		
		return range;

		/* line chart widget will do adjustment
 
		int lowerPlotMargin = getSymbolSize() + MARGIN_AT_MIN_MAX_PLOT;
		int upperPlotMargin = getSymbolSize() + MARGIN_AT_MIN_MAX_PLOT;

		return getRangeWithMargin(lowerPlotMargin, upperPlotMargin, length, axis, range);
		*/
	}

	/*
	 * @see ILineSeries#getAntialias()
	 */
	public int getAntialias()
	{
		return antialias;
	}

	/*
	 * @see ILineSeries#setAntialias(int)
	 */
	public void setAntialias(int antialias)
	{
		if (antialias != SWT.DEFAULT && antialias != SWT.ON && antialias != SWT.OFF)
		{
			SWT.error(SWT.ERROR_INVALID_ARGUMENT);
		}
		this.antialias = antialias;
	}

	/**
	 * Gets the line points to draw line and area.
	 * 
	 * @param xseries
	 *           the horizontal series
	 * @param yseries
	 *           the vertical series
	 * @param indexes
	 *           the series indexes
	 * @param index
	 *           the index of series
	 * @param xAxis
	 *           the X axis
	 * @param yAxis
	 *           the Y axis
	 * @return the line points
	 */
	private int[] getLinePoints(double[] xseries, double[] yseries, int[] indexes, int index, Axis xAxis, Axis yAxis)
	{

		int x1 = xAxis.getPixelCoordinate(xseries[index]);
		int x2 = xAxis.getPixelCoordinate(xseries[index + 1]);
		int x3 = x2;
		int x4 = x1;
		int y1 = yAxis.getPixelCoordinate(yseries[index]);
		int y2 = yAxis.getPixelCoordinate(yseries[index + 1]);
		int y3, y4;

		double baseYCoordinate = yAxis.getRange().lower > 0 ? yAxis.getRange().lower : 0;

		if (yAxis.isLogScaleEnabled())
		{
			y3 = yAxis.getPixelCoordinate(yAxis.getRange().lower);
			y4 = y3;
		}
		else if (isValidStackSeries())
		{
			y1 = yAxis.getPixelCoordinate(stackSeries[indexes[index]]);
			y2 = yAxis.getPixelCoordinate(stackSeries[indexes[index + 1]]);
			y3 = yAxis.getPixelCoordinate(stackSeries[indexes[index + 1]])
					+ Math.abs(yAxis.getPixelCoordinate(yseries[index + 1]) - yAxis.getPixelCoordinate(0))
					* (xAxis.isHorizontalAxis() ? 1 : -1);
			y4 = yAxis.getPixelCoordinate(stackSeries[indexes[index]])
					+ Math.abs(yAxis.getPixelCoordinate(yseries[index]) - yAxis.getPixelCoordinate(0))
					* (xAxis.isHorizontalAxis() ? 1 : -1);
		}
		else
		{
			y3 = yAxis.getPixelCoordinate(baseYCoordinate);
			y4 = y3;
		}

		if (xAxis.isHorizontalAxis())
		{
			return new int[] { x1, y1, x2, y2, x3, y3, x4, y4 };
		}
		return new int[] { y1, x1, y2, x2, y3, x3, y4, x4 };
	}

	/*
	 * @see Series#draw(GC, int, int, Axis, Axis)
	 */
	@Override
	protected void draw(GC gc, int width, int height, Axis xAxis, Axis yAxis)
	{
		int oldAntialias = gc.getAntialias();
		int oldLineWidth = gc.getLineWidth();
		gc.setAntialias(antialias);
		gc.setLineWidth(lineWidth);

		if (lineStyle != LineStyle.NONE)
		{
			drawLineAndArea(gc, width, height, xAxis, yAxis);
		}

		if (symbolType != PlotSymbolType.NONE || getLabel().isVisible() || getXErrorBar().isVisible() || getYErrorBar().isVisible())
		{
			drawSymbolAndLabel(gc, width, height, xAxis, yAxis);
		}

		gc.setAntialias(oldAntialias);
		gc.setLineWidth(oldLineWidth);
	}

	/**
	 * Draws the line and area.
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
	private void drawLineAndArea(GC gc, int width, int height, Axis xAxis, Axis yAxis)
	{

		// get x and y series
		double[] xseries = compressor.getCompressedXSeries();
		double[] yseries = compressor.getCompressedYSeries();
		if (xseries.length == 0 || yseries.length == 0)
		{
			return;
		}
		int[] indexes = compressor.getCompressedIndexes();
		if (xAxis.isValidCategoryAxis())
		{
			for(int i = 0; i < xseries.length; i++)
			{
				xseries[i] = indexes[i];
			}
		}

		gc.setLineStyle(Util.getIndexDefinedInSWT(lineStyle));
		gc.setForeground(getLineColor());

		boolean isHorizontal = xAxis.isHorizontalAxis();
		if (stepEnabled || areaEnabled || stackEnabled)
		{
			for(int i = 0; i < xseries.length - 1; i++)
			{
				int[] p = getLinePoints(xseries, yseries, indexes, i, xAxis, yAxis);

				// draw line
				if (lineStyle != LineStyle.NONE)
				{
					if (stepEnabled)
					{
						if (isHorizontal)
						{
							gc.drawLine(p[0], p[1], p[2], p[1]);
							gc.drawLine(p[2], p[1], p[2], p[3]);
						}
						else
						{
							gc.drawLine(p[0], p[1], p[0], p[3]);
							gc.drawLine(p[0], p[3], p[2], p[3]);
						}
					}
					else
					{
						gc.drawLine(p[0], p[1], p[2], p[3]);
					}
				}

				// draw area
				if (areaEnabled)
				{
					drawArea(gc, p, isHorizontal);
				}
			}
		}
		else
		{
			double xLower = xAxis.getRange().lower;
			double xUpper = xAxis.getRange().upper;
			double yLower = yAxis.getRange().lower;
			double yUpper = yAxis.getRange().upper;

			int prevX = xAxis.getPixelCoordinate(xseries[0], xLower, xUpper);
			int prevY = yAxis.getPixelCoordinate(yseries[0], yLower, yUpper);
			for(int i = 0; i < xseries.length - 1; i++)
			{
				int x = xAxis.getPixelCoordinate(xseries[i + 1], xLower, xUpper);
				int y = yAxis.getPixelCoordinate(yseries[i + 1], yLower, yUpper);
				if (isHorizontal)
				{
					gc.drawLine(prevX, prevY, x, y);
				}
				else
				{
					gc.drawLine(prevY, prevX, y, x);
				}
				prevX = x;
				prevY = y;
			}
		}
	}

	/**
	 * Draws the area.
	 * 
	 * @param gc
	 *           the graphic context
	 * @param p
	 *           the line points
	 * @param isHorizontal
	 *           true if orientation is horizontal
	 */
	private void drawArea(GC gc, int[] p, boolean isHorizontal)
	{
		int alpha = gc.getAlpha();
		gc.setAlpha(ALPHA);
		gc.setBackground(getLineColor());

		int[] pointArray;
		if (stepEnabled)
		{
			if (isHorizontal)
			{
				pointArray = new int[] { p[0], p[1], p[2], p[1], p[4], p[7], p[6], p[7], p[0], p[1] };
			}
			else
			{
				pointArray = new int[] { p[0], p[1], p[0], p[3], p[6], p[5], p[6], p[7], p[0], p[1] };
			}
		}
		else
		{
			pointArray = new int[] { p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[0], p[1] };
		}

		gc.fillPolygon(pointArray);
		gc.setAlpha(alpha);
	}

	/**
	 * Draws series symbol, label and error bars.
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
	private void drawSymbolAndLabel(GC gc, int width, int height, Axis xAxis, Axis yAxis)
	{

		// get x and y series
		double[] xseries = compressor.getCompressedXSeries();
		double[] yseries = compressor.getCompressedYSeries();
		int[] indexes = compressor.getCompressedIndexes();
		if (xAxis.isValidCategoryAxis())
		{
			boolean isValidStackSeries = isValidStackSeries();
			for(int i = 0; i < xseries.length; i++)
			{
				xseries[i] = indexes[i];
				if (isValidStackSeries)
				{
					yseries[i] = stackSeries[indexes[i]];
				}
			}
		}

		// draw symbol and label
		for(int i = 0; i < xseries.length; i++)
		{
			Color color;
			if (symbolColors != null && symbolColors.length > indexes[i])
			{
				color = symbolColors[indexes[i]];
			}
			else
			{
				color = getSymbolColor();
			}
			int h, v;
			if (xAxis.isHorizontalAxis())
			{
				h = xAxis.getPixelCoordinate(xseries[i]);
				v = yAxis.getPixelCoordinate(yseries[i]);
			}
			else
			{
				v = xAxis.getPixelCoordinate(xseries[i]);
				h = yAxis.getPixelCoordinate(yseries[i]);
			}
			if (getSymbolType() != PlotSymbolType.NONE)
			{
				drawSeriesSymbol(gc, h, v, color);
			}
			seriesLabel.draw(gc, h, v, yseries[i], indexes[i], SWT.BOTTOM);
			xErrorBar.draw(gc, h, v, xAxis, indexes[i]);
			yErrorBar.draw(gc, h, v, yAxis, indexes[i]);
		}
	}

	/**
	 * Draws series symbol.
	 * 
	 * @param gc
	 *           the GC object
	 * @param h
	 *           the horizontal coordinate to draw symbol
	 * @param v
	 *           the vertical coordinate to draw symbol
	 * @param color
	 *           the symbol color
	 */
	public void drawSeriesSymbol(GC gc, int h, int v, Color color)
	{
		int oldAntialias = gc.getAntialias();
		gc.setAntialias(SWT.ON);
		gc.setForeground(color);
		gc.setBackground(color);

		switch(symbolType)
		{
			case CIRCLE:
				gc.fillOval(h - symbolSize, v - symbolSize, symbolSize * 2, symbolSize * 2);
				break;
			case SQUARE:
				gc.fillRectangle(h - symbolSize, v - symbolSize, symbolSize * 2, symbolSize * 2);
				break;
			case DIAMOND:
				int[] diamondArray = { h, v - symbolSize, h + symbolSize, v, h, v + symbolSize, h - symbolSize, v };
				gc.fillPolygon(diamondArray);
				break;
			case TRIANGLE:
				int[] triangleArray = { h, v - symbolSize, h + symbolSize, v + symbolSize, h - symbolSize, v + symbolSize };
				gc.fillPolygon(triangleArray);
				break;
			case INVERTED_TRIANGLE:
				int[] invertedTriangleArray = { h, v + symbolSize, h + symbolSize, v - symbolSize, h - symbolSize, v - symbolSize };
				gc.fillPolygon(invertedTriangleArray);
				break;
			case CROSS:
				gc.setLineStyle(SWT.LINE_SOLID);
				gc.drawLine(h - symbolSize, v - symbolSize, h + symbolSize, v + symbolSize);
				gc.drawLine(h - symbolSize, v + symbolSize, h + symbolSize, v - symbolSize);
				break;
			case PLUS:
				gc.setLineStyle(SWT.LINE_SOLID);
				gc.drawLine(h, v - symbolSize, h, v + symbolSize);
				gc.drawLine(h - symbolSize, v, h + symbolSize, v);
				break;
			case NONE:
			default:
				break;
		}
		gc.setAntialias(oldAntialias);
	}
}
