/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

/**
 * A range.
 */
public class Range {

    /** the lower value of range */
    public double lower;

    /** the upper value of range */
    public double upper;

    /**
     * Constructor.
     * 
     * @param start
     *            the start value of range
     * @param end
     *            the end value of range
     */
    public Range(double start, double end) {
        this.lower = (end > start) ? start : end;
        this.upper = (end > start) ? end : start;
    }

    /**
     * Constructor.
     * 
     * @param range
     *            the range
     */
    public Range(Range range) {
        lower = (range.upper > range.lower) ? range.lower : range.upper;
        upper = (range.upper > range.lower) ? range.upper : range.lower;
    }

    /*
     * @see Object#toString()
     */
    @Override
    public String toString() {
        return "lower=" + lower + ", upper=" + upper;
    }
}
