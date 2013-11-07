/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.compress;

import java.util.ArrayList;

/**
 * A base class for compressor providing default implementations.
 */
public abstract class Compress implements ICompress {

    /** the previous X grid index */
    protected int previousXGridIndex;

    /** the previous Y grid index */
    protected int previousYGridIndex;

    /** the configuration for compressor */
    protected CompressConfig config;

    /** the previous configuration for compressor */
    protected CompressConfig prevConfig;

    /** the flag indicating whether the data is compressed */
    protected boolean compressed;

    /** the source X series to be compressed */
    protected double[] xSeries = null;

    /** the source Y series to be compressed */
    protected double[] ySeries = null;

    /** the compressed X series */
    protected transient double[] compressedXSeries = null;

    /** the compressed Y series */
    protected transient double[] compressedYSeries = null;

    /** the compressed series indexes */
    protected transient int[] compressedIndexes = null;

    /** the lower value of x range */
    protected double xLower;

    /** the upper value of x range */
    protected double xUpper;

    /** the lower value of y range */
    protected double yLower;

    /** the upper value of y range */
    protected double yUpper;

    /** the state indicating if x axis is log scale */
    private boolean isXLogScale;

    /** the state indicating if y axis is log scale */
    private boolean isYLogScale;

    /** the plot area width in pixels */
    private long widthInPixel;

    /** the plot area height in pixels */
    private long heightInPixel;

    /*
     * @see ICompress#setXSeries(double[])
     */
    public void setXSeries(double[] xSeries) {
        if (xSeries == null) {
            return;
        }

        double[] copiedSeries = new double[xSeries.length];
        System.arraycopy(xSeries, 0, copiedSeries, 0, xSeries.length);

        this.xSeries = copiedSeries;
        compressedXSeries = copiedSeries;
        compressedIndexes = new int[xSeries.length];
        for (int i = 0; i < xSeries.length; i++) {
            compressedIndexes[i] = i;
        }

        compressed = false;
    }

    /*
     * @see ICompress#setYSeries(double[])
     */
    public void setYSeries(double[] ySeries) {
        if (ySeries == null) {
            return;
        }

        double[] copiedSeries = new double[ySeries.length];
        System.arraycopy(ySeries, 0, copiedSeries, 0, ySeries.length);

        this.ySeries = copiedSeries;
        compressedYSeries = copiedSeries;

        compressed = false;
    }

    /*
     * @see ICompress#getCompressedXSeries()
     */
    public double[] getCompressedXSeries() {
        double[] copiedSeries = new double[compressedXSeries.length];
        System.arraycopy(compressedXSeries, 0, copiedSeries, 0,
                compressedXSeries.length);

        return copiedSeries;
    }

    /*
     * @see ICompress#getCompressedYSeries()
     */
    public double[] getCompressedYSeries() {
        double[] copiedSeries = new double[compressedYSeries.length];
        System.arraycopy(compressedYSeries, 0, copiedSeries, 0,
                compressedYSeries.length);

        return copiedSeries;
    }

    /*
     * @see ICompress#getCompressedIndexes()
     */
    public int[] getCompressedIndexes() {
        int[] copiedSeries = new int[compressedIndexes.length];
        System.arraycopy(compressedIndexes, 0, copiedSeries, 0,
                compressedIndexes.length);

        return copiedSeries;
    }

    /*
     * @see ICompress#compress(CompressConfig)
     */
    final public boolean compress(CompressConfig compressConfig) {

        if ((compressConfig.equals(prevConfig) && compressed)
                || xSeries == null || ySeries == null) {
            return false;
        }

        // store the previous configuration
        prevConfig = new CompressConfig(compressConfig);

        this.config = compressConfig;

        // store into fields to improve performance
        xLower = config.getXLowerValue();
        xUpper = config.getXUpperValue();
        yLower = config.getYLowerValue();
        yUpper = config.getYUpperValue();
        isXLogScale = config.isXLogScale();
        isYLogScale = config.isYLogScale();
        widthInPixel = config.getWidthInPixel();
        heightInPixel = config.getHeightInPixel();

        previousXGridIndex = -1;
        previousYGridIndex = -1;

        ArrayList<Double> xList = new ArrayList<Double>();
        ArrayList<Double> yList = new ArrayList<Double>();
        ArrayList<Integer> indexList = new ArrayList<Integer>();

        // add necessary plots to the array
        addNecessaryPlots(xList, yList, indexList);

        compressedXSeries = new double[xList.size()];
        compressedYSeries = new double[yList.size()];
        compressedIndexes = new int[indexList.size()];
        for (int i = 0; i < xList.size(); i++) {
            compressedXSeries[i] = xList.get(i);
            compressedYSeries[i] = yList.get(i);
            compressedIndexes[i] = indexList.get(i);
        }

        compressed = true;

        return true;
    }

    /**
     * Adds the necessary plots.
     * 
     * @param xList
     *            the array in which x coordinate for necessary plot is stored
     * @param yList
     *            the array in which y coordinate for necessary plot is stored
     * @param indexList
     *            the array in which series index for necessary plot is stored
     */
    abstract protected void addNecessaryPlots(ArrayList<Double> xList,
            ArrayList<Double> yList, ArrayList<Integer> indexList);

    /**
     * Adds the given coordinate to list.
     * 
     * @param xList
     *            the list to store the X coordinate
     * @param yList
     *            the list to store the Y coordinate
     * @param indexList
     *            the list to store the series index
     * @param x
     *            the X coordinate
     * @param y
     *            the Y coordinate
     * @param index
     *            the series index
     */
    protected void addToList(ArrayList<Double> xList, ArrayList<Double> yList,
            ArrayList<Integer> indexList, double x, double y, int index) {
        xList.add(x);
        yList.add(y);
        indexList.add(index);
    }

    /**
     * Checks if the given coordinate is in the same grid as previous.
     * 
     * @param x
     *            the X coordinate
     * @param y
     *            the Y coordinate
     * @return true if the given coordinate is in the same grid as previous
     */
    protected boolean isInSameGridAsPrevious(double x, double y) {
        int xGridIndex;
        int yGridIndex;

        // calculate the X grid index
        if (isXLogScale) {
            double lower = Math.log10(xLower);
            double upper = Math.log10(xUpper);
            xGridIndex = (int) ((Math.log10(x) - lower) / (upper - lower) * widthInPixel);
        } else {
            xGridIndex = (int) ((x - xLower) / (xUpper - xLower) * widthInPixel);
        }

        // calculate the Y grid index
        if (isYLogScale) {
            double lower = Math.log10(yLower);
            double upper = Math.log10(yUpper);
            yGridIndex = (int) ((Math.log10(y) - lower) / (upper - lower) * heightInPixel);
        } else {
            yGridIndex = (int) ((y - yLower) / (yUpper - yLower) * heightInPixel);
        }

        // check if the grid index is the same as previous
        boolean isInSameGridAsPrevious = (xGridIndex == previousXGridIndex && yGridIndex == previousYGridIndex);

        // store the previous grid index
        previousXGridIndex = xGridIndex;
        previousYGridIndex = yGridIndex;

        return isInSameGridAsPrevious;
    }
}
