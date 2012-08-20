/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

/**
 * A line style.
 */
public enum LineStyle {

    /** none */
    NONE("None"),

    /** solid */
    SOLID("Solid"),

    /** dash */
    DASH("Dash"),

    /** dot */
    DOT("Dot"),

    /** dash dot */
    DASHDOT("Dash Dot"),

    /** dash dot dot */
    DASHDOTDOT("Dash Dot Dot");

    /** the label for line style */
    public final String label;

    /**
     * Constructor.
     * 
     * @param label
     *            the label for line style
     */
    private LineStyle(String label) {
        this.label = label;
    }
}