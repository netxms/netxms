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
import org.swtchart.IErrorBar;
import org.swtchart.internal.axis.Axis;

/**
 * The error bar.
 */
public class ErrorBar implements IErrorBar {

    /** the default line width */
    private static final int DEFAULT_LINE_WIDTH = 1;

    /** the default error */
    private static final double DEFAULT_ERROR = 1d;

    /** the default color */
    private static final int DEFAULT_COLOR = SWT.COLOR_DARK_GRAY;

    /** the default error bar type */
    private static final ErrorBarType DEFAULT_TYPE = ErrorBarType.BOTH;

    /** the color */
    private Color color;

    /** the line width */
    private int lineWidth;

    /** the error */
    private double error;

    /** the plus errors */
    private double[] plusErrors;

    /** the minus errors */
    private double[] minusErrors;

    /** the error bar type */
    private ErrorBarType type;

    /** the visibility state */
    private boolean isVisible;

    /**
     * The constructor.
     */
    public ErrorBar() {
        color = Display.getDefault().getSystemColor(DEFAULT_COLOR);
        lineWidth = DEFAULT_LINE_WIDTH;
        error = DEFAULT_ERROR;
        type = ErrorBarType.BOTH;
        isVisible = false;
    }

    /*
     * @see IErrorBar#getType()
     */
    public ErrorBarType getType() {
        return type;
    }

    /*
     * @see IErrorBar#setType(ErrorBarType)
     */
    public void setType(ErrorBarType type) {
        if (type == null) {
            this.type = DEFAULT_TYPE;
        } else {
            this.type = type;
        }
    }

    /*
     * @see IErrorBar#getColor()
     */
    public Color getColor() {
        if (color.isDisposed()) {
            color = Display.getDefault().getSystemColor(DEFAULT_COLOR);
        }
        return color;
    }

    /*
     * @see IErrorBar#setColor(Color)
     */
    public void setColor(Color color) {
        if (color != null && color.isDisposed()) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }

        if (color == null) {
            this.color = Display.getDefault().getSystemColor(DEFAULT_COLOR);
        } else {
            this.color = color;
        }
    }

    /*
     * @see IErrorBar#getLineWidth()
     */
    public int getLineWidth() {
        return lineWidth;
    }

    /*
     * @see IErrorBar#setLineWidth(int)
     */
    public void setLineWidth(int width) {
        if (width <= 0) {
            this.lineWidth = DEFAULT_LINE_WIDTH;
        } else {
            this.lineWidth = width;
        }
    }

    /*
     * @see IErrorBar#getError()
     */
    public double getError() {
        return error;
    }

    /*
     * @see IErrorBar#setError(double)
     */
    public void setError(double error) {
        if (error < 0) {
            throw new IllegalArgumentException(
                    "positive value must be given for error.");
        }

        this.error = error;
    }

    /*
     * @see IErrorBar#getPlusErrors()
     */
    public double[] getPlusErrors() {
        if (plusErrors == null) {
            return null;
        }

        double[] copiedSeries = new double[plusErrors.length];
        System.arraycopy(plusErrors, 0, copiedSeries, 0, plusErrors.length);

        return copiedSeries;
    }

    /*
     * @see IErrorBar#setPlusErrors(double[])
     */
    public void setPlusErrors(double[] errors) {
        if (errors == null) {
            SWT.error(SWT.ERROR_NULL_ARGUMENT);
            return; // to suppress warning...
        }

        this.plusErrors = new double[errors.length];
        System.arraycopy(errors, 0, plusErrors, 0, errors.length);
    }

    /*
     * @see IErrorBar#getMinusErrors()
     */
    public double[] getMinusErrors() {
        if (minusErrors == null) {
            return null;
        }

        double[] copiedSeries = new double[minusErrors.length];
        System.arraycopy(minusErrors, 0, copiedSeries, 0, minusErrors.length);

        return copiedSeries;
    }

    /*
     * @see IErrorBar#setMinusErrors(double[])
     */
    public void setMinusErrors(double[] errors) {
        if (errors == null) {
            SWT.error(SWT.ERROR_NULL_ARGUMENT);
            return; // to suppress warning...
        }

        this.minusErrors = new double[errors.length];
        System.arraycopy(errors, 0, minusErrors, 0, errors.length);
    }

    /*
     * @see IErrorBar#isVisible()
     */
    public boolean isVisible() {
        return isVisible;
    }

    /*
     * @see IErrorBar#setVisible(boolean)
     */
    public void setVisible(boolean visible) {
        this.isVisible = visible;
    }

    /**
     * Draws error bar.
     * 
     * @param gc
     *            the graphics context
     * @param h
     *            the horizontal coordinate to draw error bar
     * @param v
     *            the vertical coordinate to draw error bar
     * @param axis
     *            the x axis
     * @param seriesIndex
     *            the series index
     */
    protected void draw(GC gc, int h, int v, Axis axis, int seriesIndex) {
        if (!isVisible) {
            return;
        }

        int oldLineWidth = gc.getLineWidth();
        gc.setLineWidth(lineWidth);
        gc.setLineStyle(SWT.LINE_SOLID);
        gc.setForeground(getColor());

        // get plus/minus error
        double plusError = error;
        double minusError = error;
        if (plusErrors != null && plusErrors.length > seriesIndex) {
            plusError = plusErrors[seriesIndex];
        }
        if (minusErrors != null && minusErrors.length > seriesIndex) {
            minusError = minusErrors[seriesIndex];
        }

        // draw error bar
        draw(gc, h, v, axis, plusError, minusError);

        gc.setLineWidth(oldLineWidth);
    }

    /**
     * Draws the error bar.
     * 
     * @param gc
     *            the graphics context
     * @param h
     *            the horizontal coordinate to draw error bar
     * @param v
     *            the vertical coordinate to draw error bar
     * @param axis
     *            the axis
     * @param plusError
     *            the plus error
     * @param minusError
     *            the minus error
     */
    private void draw(GC gc, int h, int v, Axis axis, double plusError,
            double minusError) {
        if (axis.isHorizontalAxis()) {
            double dataCoordinate = axis.getDataCoordinate(h);
            int plusErrorInPixels = axis.getPixelCoordinate(dataCoordinate
                    + plusError)
                    - h;
            int miusErrorInPixels = h
                    - axis.getPixelCoordinate(dataCoordinate - minusError);
            if (axis.isLogScaleEnabled() && dataCoordinate - plusError < 0) {
                miusErrorInPixels = h
                        - axis.getPixelCoordinate(axis.getRange().lower);
            }

            if (type != ErrorBarType.MINUS) {
                gc.drawLine(h, v, h + plusErrorInPixels, v);
                gc.drawLine(h + plusErrorInPixels, v + 1 + lineWidth, h
                        + plusErrorInPixels, v - 1 - lineWidth);
            }
            if (type != ErrorBarType.PLUS) {
                gc.drawLine(h - miusErrorInPixels, v, h, v);
                gc.drawLine(h - miusErrorInPixels, v + 1 + lineWidth, h
                        - miusErrorInPixels, v - 1 - lineWidth);
            }
        } else {
            double dataCoordinate = axis.getDataCoordinate(v);
            int plusErrorInPixels = v
                    - axis.getPixelCoordinate(dataCoordinate + plusError);
            int miusErrorInPixels = axis.getPixelCoordinate(dataCoordinate
                    - minusError)
                    - v;
            if (axis.isLogScaleEnabled() && dataCoordinate - plusError < 0) {
                miusErrorInPixels = axis
                        .getPixelCoordinate(axis.getRange().lower)
                        - v;
            }

            if (type != ErrorBarType.MINUS) {
                gc.drawLine(h, v - plusErrorInPixels, h, v);
                gc.drawLine(h + 1 + lineWidth, v - plusErrorInPixels, h - 1
                        - lineWidth, v - plusErrorInPixels);
            }
            if (type != ErrorBarType.PLUS) {
                gc.drawLine(h, v, h, v + miusErrorInPixels);
                gc.drawLine(h + 1 + lineWidth, v + miusErrorInPixels, h - 1
                        - lineWidth, v + miusErrorInPixels);
            }
        }
    }
}
