/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.compress;

/**
 * Configuration for compression.
 */
public class CompressConfig {

    /** the width in pixels */
    private long widthInPixels;

    /** the height in pixels */
    private long heightInPixels;

    /** the lower value of X range */
    private double xLowerValue;

    /** the upper value of X range */
    private double xUpperValue;

    /** the lower value of Y range */
    private double yLowerValue;

    /** the upper value of Y range */
    private double yUpperValue;

    /** the state indicating whether the X axis is log scale */
    private boolean xLogScale;

    /** the state indicating whether the Y axis is log scale */
    private boolean yLogScale;

    /**
     * Constructor.
     */
    public CompressConfig() {
        widthInPixels = 1024;
        heightInPixels = 512;
        xLogScale = false;
        yLogScale = false;
    }

    /**
     * Constructor.
     * 
     * @param config
     *            the configuration for compression
     */
    public CompressConfig(CompressConfig config) {
        widthInPixels = config.getWidthInPixel();
        heightInPixels = config.getHeightInPixel();

        xLowerValue = config.getXLowerValue();
        xUpperValue = config.getXUpperValue();
        yLowerValue = config.getYLowerValue();
        yUpperValue = config.getYUpperValue();

        xLogScale = config.isXLogScale();
        yLogScale = config.isYLogScale();
    }

    /*
     * @see Object#equals(Object)
     */
    @Override
    public boolean equals(Object obj) {
        if (obj == null || !(obj instanceof CompressConfig)) {
            return false;
        }

        CompressConfig config = (CompressConfig) obj;

        if (widthInPixels != config.getWidthInPixel()
                || heightInPixels != config.getHeightInPixel()) {
            return false;
        }

        double diff = Math.abs(xLowerValue - config.getXLowerValue())
                / (xUpperValue - xLowerValue);
        if (diff > 1.0 / widthInPixels) {
            return false;
        }

        diff = Math.abs(xUpperValue - config.getXUpperValue())
                / (xUpperValue - xLowerValue);
        if (diff > 1.0 / widthInPixels) {
            return false;
        }

        diff = Math.abs(yLowerValue - config.getYLowerValue())
                / (yUpperValue - yLowerValue);
        if (diff > 1.0 / heightInPixels) {
            return false;
        }

        diff = Math.abs(yUpperValue - config.getYUpperValue())
                / (yUpperValue - yLowerValue);
        if (diff > 1.0 / heightInPixels) {
            return false;
        }

        if (config.isXLogScale() != xLogScale) {
            return false;
        }

        if (config.isYLogScale() != yLogScale) {
            return false;
        }

        return true;
    }

    /*
     * @see Object#hashCode()
     */
    @Override
    public int hashCode() {
        return 0;
    }

    /**
     * Sets the size in pixels.
     * 
     * @param width
     *            the width in pixels
     * @param height
     *            the height in pixels
     */
    public void setSizeInPixel(long width, long height) {
        widthInPixels = width;
        heightInPixels = height;
    }

    /**
     * Gets the width of plot area in pixels
     * 
     * @return the width of plot area in pixels
     */
    public long getWidthInPixel() {
        return widthInPixels;
    }

    /**
     * Gets the height of plot area in pixels
     * 
     * @return the height of plot area in pixels
     */
    public long getHeightInPixel() {
        return heightInPixels;
    }

    /**
     * Sets the X range.
     * 
     * @param lower
     *            the lower value of x range
     * @param upper
     *            the upper value of x range
     */
    public void setXRange(double lower, double upper) {
        xLowerValue = lower;
        xUpperValue = upper;
    }

    /**
     * Sets the Y range.
     * 
     * @param lower
     *            the lower value of y range
     * @param upper
     *            the upper value of y range
     */
    public void setYRange(double lower, double upper) {
        yLowerValue = lower;
        yUpperValue = upper;
    }

    /**
     * Gets the lower value of x range.
     * 
     * @return the lower value of x range
     */
    public double getXLowerValue() {
        return xLowerValue;
    }

    /**
     * Gets the upper value of x range.
     * 
     * @return the upper value of x range
     */
    public double getXUpperValue() {
        return xUpperValue;
    }

    /**
     * Gets the lower value of y range.
     * 
     * @return the lower value of y range
     */
    public double getYLowerValue() {
        return yLowerValue;
    }

    /**
     * Gets the upper value of y range.
     * 
     * @return the upper value of y range
     */
    public double getYUpperValue() {
        return yUpperValue;
    }

    /**
     * Gets the state indicating whether the X axis is log scale.
     * 
     * @return true if the X axis is log scale
     */
    public boolean isXLogScale() {
        return xLogScale;
    }

    /**
     * Sets the state indicating whether the X axis is log scale.
     * 
     * @param value
     *            the state indicating whether the X axis is log scale
     */
    public void setXLogScale(boolean value) {
        this.xLogScale = value;
    }

    /**
     * Gets the state indicating whether the Y axis is log scale.
     * 
     * @return true if the Y axis is log scale
     */
    public boolean isYLogScale() {
        return yLogScale;
    }

    /**
     * Sets the state indicating whether the Y axis is log scale.
     * 
     * @param value
     *            the state indicating whether the Y axis is log scale
     */
    public void setYLogScale(boolean value) {
        this.yLogScale = value;
    }

    /*
     * @see Object#toString()
     */
    @Override
    public String toString() {
        return "pixelWidth = " + widthInPixels + ", " + "pixelHeight = "
                + heightInPixels + ", " + "xLowerValue = " + xLowerValue + ", "
                + "xUpperValue = " + xUpperValue + ", " + "yLowerValue = "
                + yLowerValue + ", " + "yUpperValue = " + yUpperValue + ", "
                + yLogScale;
    }
}
