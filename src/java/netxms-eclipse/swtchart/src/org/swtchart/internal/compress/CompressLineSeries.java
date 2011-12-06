/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.compress;

import java.util.ArrayList;

/**
 * A compressor for line series data.
 */
public class CompressLineSeries extends Compress {

    /** the state indicating the relation between previous and current data points */
    enum STATE {
        /** stepping over x range */
        SteppingOverXRange,

        /** stepping over y range */
        SteppingOverYRange,

        /** out of range again */
        OutOfRangeAgain,

        /** stepping out of x range */
        SteppingOutOfXRange,

        /** stepping in x range */
        SteppingInXRange,

        /** stepping out of y range */
        SteppingOutOfYRange,

        /** stepping out of range */
        SteppingOutOfRange,

        /** in range again */
        InRangeAgain,

        /** stepping in range */
        SteppingInRange;
    }

    /** the flag indicating whether the previous point is out of range */
    private boolean isPrevOutOfRange;

    /*
     * @see Compress#addNecessaryPlots(ArrayList, ArrayList, ArrayList)
     */
    @Override
    protected void addNecessaryPlots(ArrayList<Double> xList,
            ArrayList<Double> yList, ArrayList<Integer> indexList) {

        isPrevOutOfRange = true;

        for (int i = 0; i < xSeries.length; i++) {
            STATE state = getState(i);

            switch (state) {
            case SteppingOutOfYRange:
                addToList(xList, yList, indexList, xSeries[i], ySeries[i], i);
                break;
            case SteppingOverYRange:
            case SteppingInRange:
            case SteppingInXRange:
                addToList(xList, yList, indexList, xSeries[i - 1],
                        ySeries[i - 1], i - 1);
                addToList(xList, yList, indexList, xSeries[i], ySeries[i], i);
                break;
            case SteppingOverXRange:
            case SteppingOutOfXRange:
                addToList(xList, yList, indexList, xSeries[i - 1],
                        ySeries[i - 1], i - 1);
                addToList(xList, yList, indexList, xSeries[i], ySeries[i], i);
                i = xSeries.length;
                break;
            case SteppingOutOfRange:
                addToList(xList, yList, indexList, xSeries[i], ySeries[i], i);
                i = xSeries.length;
                break;
            case InRangeAgain:
                if (!isInSameGridAsPrevious(xSeries[i], ySeries[i])) {
                    addToList(xList, yList, indexList, xSeries[i], ySeries[i],
                            i);
                }
                break;
            case OutOfRangeAgain:
                break;
            default:
                break;
            }
        }
    }

    /**
     * Gets the state for each plot.
     * 
     * @param index
     *            the index for plot
     * @return the state of plot for the given index
     */
    private STATE getState(int index) {

        STATE state;

        if (xLower <= xSeries[index] && xSeries[index] <= xUpper) {
            if (yLower <= ySeries[index] && ySeries[index] <= yUpper) {
                if (index > 0 && isPrevOutOfRange) {
                    state = STATE.SteppingInRange;
                } else {
                    state = STATE.InRangeAgain;
                }
            } else {
                if (isPrevOutOfRange) {
                    if (index > 0 && ySeries[index - 1] < yLower
                            && ySeries[index] > yUpper) {
                        state = STATE.SteppingOverYRange;
                    } else if (index > 0 && xSeries[index - 1] < xLower
                            && xSeries[index] > xLower) {
                        state = STATE.SteppingInXRange;
                    } else {
                        state = STATE.OutOfRangeAgain;
                    }
                } else {
                    state = STATE.SteppingOutOfYRange;
                }
            }
        } else {
            if (!isPrevOutOfRange) {
                state = STATE.SteppingOutOfRange;
            } else if (index > 0 && xSeries[index - 1] < xUpper
                    && xSeries[index] > xUpper) {
                state = STATE.SteppingOutOfXRange;
            } else if (index > 0 && xSeries[index - 1] < xLower
                    && xSeries[index] > xUpper) {
                state = STATE.SteppingOverXRange;
            } else {
                state = STATE.OutOfRangeAgain;
            }
        }

        // set flag
        if (xLower <= xSeries[index] && xSeries[index] <= xUpper
                && yLower <= ySeries[index] && ySeries[index] <= yUpper) {
            isPrevOutOfRange = false;
        } else {
            isPrevOutOfRange = true;
        }

        return state;
    }
}
