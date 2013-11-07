/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.compress;

import java.util.ArrayList;

/**
 * A compressor for scatter series data
 */
public class CompressScatterSeries extends Compress {

    /** the state indicating if line is visible */
    private boolean isLineVisible;

    /** flag indicating whether the grid is occupied */
    private boolean occupied[][];

    /*
     * @see Compress#addNecessaryPlots(ArrayList, ArrayList, ArrayList)
     */
    @Override
    protected void addNecessaryPlots(ArrayList<Double> xList,
            ArrayList<Double> yList, ArrayList<Integer> indexList) {

        if (isLineVisible) {
            for (int i = 0; i < xSeries.length; i++) {
                if (!isInSameGridAsPrevious(xSeries[i], ySeries[i])) {
                    addToList(xList, yList, indexList, xSeries[i], ySeries[i],
                            i);
                }
            }
        } else {
            int width = (int) config.getWidthInPixel();
            int height = (int) config.getHeightInPixel();

            if (width <= 0 || height <= 0) {
                return;
            }

            // initialize flag
            occupied = new boolean[width][height];

            for (int i = 0; i < xSeries.length; i++) {
                if (xSeries[i] >= xLower && xSeries[i] <= xUpper
                        && ySeries[i] >= yLower && ySeries[i] <= yUpper
                        && !isOccupied(xSeries[i], ySeries[i])) {
                    addToList(xList, yList, indexList, xSeries[i], ySeries[i],
                            i);
                }
            }
        }
    }

    /**
     * check if the grid is already occupied
     * 
     * @param x
     *            the X coordinate
     * @param y
     *            the Y coordinate
     * @return true if the grid is already occupied
     */
    private boolean isOccupied(double x, double y) {
        int xGridIndex;
        int yGridIndex;

        // calculate the X grid index
        if (config.isXLogScale()) {
            double lower = Math.log10(config.getXLowerValue());
            double upper = Math.log10(config.getXUpperValue());
            xGridIndex = (int) ((Math.log10(x) - lower) / (upper - lower) * config
                    .getWidthInPixel());
        } else {
            xGridIndex = (int) ((x - config.getXLowerValue())
                    / (config.getXUpperValue() - config.getXLowerValue()) * config
                    .getWidthInPixel());
        }

        // calculate the Y grid index
        if (config.isYLogScale()) {
            double lower = Math.log10(config.getYLowerValue());
            double upper = Math.log10(config.getYUpperValue());
            yGridIndex = (int) ((Math.log10(y) - lower) / (upper - lower) * config
                    .getHeightInPixel());
        } else {
            yGridIndex = (int) ((y - config.getYLowerValue())
                    / (config.getYUpperValue() - config.getYLowerValue()) * config
                    .getHeightInPixel());
        }

        boolean isOccupied = occupied[xGridIndex][yGridIndex];

        occupied[xGridIndex][yGridIndex] = true;

        return isOccupied;
    }

    /**
     * Sets the state indicating if the line is visible.
     * 
     * @param visible
     *            the state indicating if the line is visible
     */
    public void setLineVisible(boolean visible) {
        isLineVisible = visible;
    }
}
