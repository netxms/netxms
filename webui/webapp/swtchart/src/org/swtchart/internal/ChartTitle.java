/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved.
 *
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal;

import org.swtchart.Chart;

/**
 * A chart title.
 */
public class ChartTitle extends Title
{

    /** the default text */
    private static final String DEFAULT_TEXT = "Chart Title";

    /**
     * Constructor.
     *
     * @param chart
     *            the plot chart
     * @param style
     *            the style
     */
    public ChartTitle(Chart chart, int style) {
        super(chart, style);
        setText(getDefaultText());
    }

    /*
     * @see Title#getDefaultText()
     */
    @Override
    protected String getDefaultText() {
        return DEFAULT_TEXT;
    }
}
