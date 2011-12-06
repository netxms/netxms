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
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Display;
import org.swtchart.Chart;
import org.swtchart.IBarSeries;
import org.swtchart.Range;
import org.swtchart.IAxis.Direction;
import org.swtchart.internal.axis.Axis;
import org.swtchart.internal.compress.CompressBarSeries;
import org.swtchart.internal.compress.CompressScatterSeries;

/**
 * Bar series.
 */
public class BarSeries extends Series implements IBarSeries {

    /** the riser index in a category */
    private int riserIndex;

    /** the riser color */
    private Color barColor;

    /** the padding */
    private int padding;

    /** the initial bar padding in percentage */
    public static final int INITIAL_PADDING = 20;

    /** the alpha value */
    private static final int ALPHA = 0xD0;

    /** the margin in pixels attached at the minimum/maximum plot */
    private static final int MARGIN_AT_MIN_MAX_PLOT = 6;

    /** the default bar color */
    private static final int DEFAULT_BAR_COLOR = SWT.COLOR_CYAN;

    /**
     * Constructor.
     * 
     * @param chart
     *            the chart
     * @param id
     *            the series id
     */
    protected BarSeries(Chart chart, String id) {
        super(chart, id);

        barColor = Display.getDefault().getSystemColor(DEFAULT_BAR_COLOR);
        padding = INITIAL_PADDING;
        type = SeriesType.BAR;

        compressor = new CompressBarSeries();
    }

    /*
     * @see IBarSeries#getBarPadding()
     */
    public int getBarPadding() {
        return padding;
    }

    /*
     * @see IBarSeries#setBarPadding(int)
     */
    public void setBarPadding(int padding) {
        if (padding < 0 || padding > 100) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }
        this.padding = padding;
    }

    /*
     * @see IBarSeries#getBarColor()
     */
    public Color getBarColor() {
        if (barColor.isDisposed()) {
            barColor = Display.getDefault().getSystemColor(DEFAULT_BAR_COLOR);
        }
        return barColor;
    }

    /*
     * @see IBarSeries#setBarColor(Color)
     */
    public void setBarColor(Color color) {
        if (color != null && color.isDisposed()) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }

        if (color == null) {
            this.barColor = Display.getDefault().getSystemColor(
                    DEFAULT_BAR_COLOR);
        } else {
            this.barColor = color;
        }
    }

    /*
     * @see IBarSeries#getBounds()
     */
    public Rectangle[] getBounds() {
        Rectangle[] compressedBounds = getBoundsForCompressedSeries();
        if (((Axis) chart.getAxisSet().getXAxis(xAxisId)).isValidCategoryAxis()) {
            return compressedBounds;
        }

        Rectangle[] rs = new Rectangle[xSeries.length];
        double[] comporessedXSeries = compressor.getCompressedXSeries();
        int cnt = 0;
        for (int i = 0; i < xSeries.length; i++) {
            if (cnt < comporessedXSeries.length
                    && comporessedXSeries[cnt] == xSeries[i]) {
                rs[i] = compressedBounds[cnt++];
            }
        }
        return rs;
    }

    /**
     * Gets the array of bar rectangles for compressed series.
     * 
     * @return the array of bar rectangles for compressed series
     */
    private Rectangle[] getBoundsForCompressedSeries() {
        Axis xAxis = (Axis) chart.getAxisSet().getXAxis(xAxisId);
        Axis yAxis = (Axis) chart.getAxisSet().getYAxis(yAxisId);

        // get x and y series
        double[] xseries = compressor.getCompressedXSeries();
        double[] yseries = compressor.getCompressedYSeries();
        int[] indexes = compressor.getCompressedIndexes();
        if (xAxis.isValidCategoryAxis()) {
            for (int i = 0; i < xseries.length; i++) {
                xseries[i] = indexes[i];
            }
        }

        Rectangle[] rectangles = new Rectangle[xseries.length];
        Range xRange = xAxis.getRange();
        Range yRange = yAxis.getRange();
        for (int i = 0; i < xseries.length; i++) {
            int x = xAxis.getPixelCoordinate(xseries[i]);
            int y = yAxis
                    .getPixelCoordinate(isValidStackSeries() ? stackSeries[indexes[i]]
                            : yseries[i]);
            double baseYCoordinate = yAxis.getRange().lower > 0 ? yAxis
                    .getRange().lower : 0;
            double riserwidth = getRiserWidth(xseries, i, xAxis, xRange.lower,
                    xRange.upper);
            double riserHeight = Math.abs(yAxis.getPixelCoordinate(yseries[i],
                    yRange.lower, yRange.upper)
                    - yAxis.getPixelCoordinate(
                            yAxis.isLogScaleEnabled() ? yRange.lower
                                    : baseYCoordinate, yRange.lower,
                            yRange.upper));

            // adjust riser x coordinate and riser width for multiple series
            int riserCnt = xAxis.getNumRisers();
            if (riserCnt > 1) {
                if (xAxis.isHorizontalAxis()) {
                    x = (int) (x - riserwidth / 2d + riserwidth / riserCnt
                            * (riserIndex + 0.5));
                } else {
                    x = (int) (x - riserwidth / 2d + riserwidth / riserCnt
                            * (riserCnt - riserIndex - 0.5));
                }
                riserwidth /= riserCnt;
            }

            if (xAxis.isHorizontalAxis()) {

                // adjust coordinate for negative series
                if (y > yAxis.getPixelCoordinate(0)) {
                    y = yAxis.getPixelCoordinate(0);
                }

                int width = (int) Math.ceil(riserwidth);
                width = (width == 0) ? 1 : width;

                rectangles[i] = getVisibleRectangle((int) Math.floor(x
                        - riserwidth / 2d), y, width, (int) riserHeight);
            } else {

                // adjust coordinate for negative series
                if (y < yAxis.getPixelCoordinate(0)) {
                    y = yAxis.getPixelCoordinate(0);
                }

                int height = (int) Math.ceil(riserwidth);
                height = (height == 0) ? 1 : height;

                rectangles[i] = getVisibleRectangle((int) (y - riserHeight),
                        (int) Math.floor(x - riserwidth / 2d),
                        (int) riserHeight, height);
            }
        }

        return rectangles;
    }

    /**
     * Gets the rectangle that is visible part of given rectangle.
     * 
     * @param x
     *            The x coordinate
     * @param y
     *            The y coordinate
     * @param width
     *            the width
     * @param height
     *            The height
     * @return The visible rectangle
     */
    private Rectangle getVisibleRectangle(int x, int y, int width, int height) {

        final int offset = 5;
        int newX = x;
        int newY = y;
        int newWidth = width;
        int newHeight = height;

        if (x < 0) {
            newX = -offset;
            newWidth += x + offset;
        }
        if (y < 0) {
            newY = -offset;
            newHeight += y + offset;
        }

        Point size = chart.getPlotArea().getSize();
        if (x + width > size.x) {
            newWidth -= x + width - size.x + offset;
        }
        if (y + height > size.y) {
            newHeight -= y + height - size.y + offset;
        }

        return new Rectangle(newX, newY, newWidth, newHeight);
    }

    /**
     * Sets the index of riser in a category.
     * 
     * @param riserIndex
     *            the index of riser in a category
     */
    protected void setRiserIndex(int riserIndex) {
        this.riserIndex = riserIndex;
    }

    /*
     * @see Series#setCompressor()
     */
    @Override
    protected void setCompressor() {
        if (isXMonotoneIncreasing) {
            compressor = new CompressBarSeries();
        } else {
            compressor = new CompressScatterSeries();
        }
    }

    /*
     * @see Series#getAdjustedRange(Axis, int)
     */
    @Override
    public Range getAdjustedRange(Axis axis, int length) {

        // calculate a range which has margin
        Range range;
        int lowerPlotMargin;
        int upperPlotMargin;
        if (axis.getDirection() == Direction.X) {
            double lowerRiserWidth = getRiserWidth(xSeries, 0, axis, minX, maxX);
            double upperRiserWidth = getRiserWidth(xSeries, xSeries.length - 1,
                    axis, minX, maxX);
            lowerPlotMargin = (int) (lowerRiserWidth / 2d + MARGIN_AT_MIN_MAX_PLOT);
            upperPlotMargin = (int) (upperRiserWidth / 2d + MARGIN_AT_MIN_MAX_PLOT);
            range = getXRange();
        } else {
            range = getYRange();
            if (range.upper < 0) {
                range.upper = 0;
            }
            if (range.lower > 0) {
                range.lower = axis.isLogScaleEnabled() ? minY : 0;
            }
            lowerPlotMargin = (range.lower == 0) ? 0 : MARGIN_AT_MIN_MAX_PLOT;
            upperPlotMargin = (range.upper == 0) ? 0 : MARGIN_AT_MIN_MAX_PLOT;
        }

        return getRangeWithMargin(lowerPlotMargin, upperPlotMargin, length,
                axis, range);
    }

    /**
     * Gets the riser width.
     * 
     * @param series
     *            the X series
     * @param index
     *            the series index
     * @param xAxis
     *            the X axis
     * @param min
     *            the min value of range
     * @param max
     *            the max value of range
     * @return the raiser width in pixels
     */
    private int getRiserWidth(double[] series, int index, Axis xAxis,
            double min, double max) {

        // get two x coordinates
        double upper;
        double lower;
        if (series.length == 1) {
            upper = series[0] + 0.5;
            lower = series[0] - 0.5;
        } else if (index != series.length - 1
                && (index == 0 || series[index + 1] - series[index] < series[index]
                        - series[index - 1])) {
            upper = series[index + 1];
            lower = series[index];
        } else {
            upper = series[index];
            lower = series[index - 1];
        }

        // get riser width without padding
        int width = Math.abs(xAxis.getPixelCoordinate(upper, min, max)
                - xAxis.getPixelCoordinate(lower, min, max));

        // adjust for padding
        width *= (100 - padding) / 100d;

        // symbol size should be at least more than 1
        if (width == 0) {
            width = 1;
        }

        return width;
    }

    /**
     * Gets the color for riser frame. The color will be darker or lighter than
     * the given color.
     * 
     * @param color
     *            the riser color
     * @return the riser frame color
     */
    private Color getFrameColor(Color color) {
        int red = color.getRed();
        int green = color.getGreen();
        int blue = color.getBlue();

        red *= (red > 128) ? 0.8 : 1.2;
        green *= (green > 128) ? 0.8 : 1.2;
        blue *= (blue > 128) ? 0.8 : 1.2;

        return new Color(color.getDevice(), red, green, blue);
    }

    /*
     * @see Series#draw(GC, int, int, Axis, Axis)
     */
    @Override
    protected void draw(GC gc, int width, int height, Axis xAxis, Axis yAxis) {

        // draw riser
        Rectangle[] rs = getBoundsForCompressedSeries();
        for (int i = 0; i < rs.length; i++) {
            drawRiser(gc, rs[i].x, rs[i].y, rs[i].width, rs[i].height);
        }

        // draw label and error bars
        if (seriesLabel.isVisible() || xErrorBar.isVisible()
                || yErrorBar.isVisible()) {
            double[] yseries = compressor.getCompressedYSeries();
            int[] indexes = compressor.getCompressedIndexes();

            for (int i = 0; i < rs.length; i++) {
                seriesLabel.draw(gc, rs[i].x + rs[i].width / 2, rs[i].y
                        + rs[i].height / 2, yseries[i], indexes[i], SWT.CENTER);

                int h, v;
                if (xAxis.isHorizontalAxis()) {
                    h = xAxis.getPixelCoordinate(xSeries[indexes[i]]);
                    v = yAxis.getPixelCoordinate(ySeries[indexes[i]]);
                } else {
                    v = xAxis.getPixelCoordinate(xSeries[indexes[i]]);
                    h = yAxis.getPixelCoordinate(ySeries[indexes[i]]);
                }
                xErrorBar.draw(gc, h, v, xAxis, indexes[i]);
                yErrorBar.draw(gc, h, v, yAxis, indexes[i]);
            }
        }
    }

    /**
     * Draws riser.
     * 
     * @param gc
     *            the graphics context
     * @param h
     *            the horizontal coordinate
     * @param v
     *            the vertical coordinate
     * @param width
     *            the riser width
     * @param height
     *            the riser height
     */
    private void drawRiser(GC gc, int h, int v, int width, int height) {
        int alpha = gc.getAlpha();
        gc.setAlpha(ALPHA);

        gc.setBackground(getBarColor());
        gc.fillRectangle(h, v, width, height);

        gc.setLineStyle(SWT.LINE_SOLID);
        Color frameColor = getFrameColor(getBarColor());
        gc.setForeground(frameColor);
        gc.drawRectangle(h, v, width, height);
        frameColor.dispose();

        gc.setAlpha(alpha);
    }
}
