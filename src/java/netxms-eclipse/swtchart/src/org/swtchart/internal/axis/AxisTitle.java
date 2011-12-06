/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved.
 *
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.axis;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.widgets.Display;
import org.swtchart.Chart;
import org.swtchart.Constants;
import org.swtchart.IAxis.Direction;
import org.swtchart.internal.Title;

/**
 * An Axis title.
 */
public class AxisTitle extends Title {

    /** the default text for X Axis */
    private static final String DEFAULT_TEXT_FOR_XAXIS = "X Axis";

    /** the default text for X Axis */
    private static final String DEFAULT_TEXT_FOR_YAXIS = "Y Axis";

    /** the default color */
    private static final int DEFAULT_FONT_SIZE = Constants.MEDIUM_FONT_SIZE;

    /** the default font */
    private final Font defaultFont;

    /** the axis */
    private final Axis axis;

    /** the direction of axis */
    private final Direction direction;

    /**
     * Constructor.
     *
     * @param chart
     *            the chart
     * @param style
     *            the style
     * @param axis
     *            the axis
     * @param direction
     *            the direction
     */
    public AxisTitle(Chart chart, int style, Axis axis, Direction direction) {
        super(chart, style);
        this.axis = axis;
        this.direction = direction;
        defaultFont = new Font(Display.getDefault(), "Tahoma",
                DEFAULT_FONT_SIZE, SWT.BOLD);
        setFont(defaultFont);
        setText(getDefaultText());
        addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
            defaultFont.dispose();
			}
		});
    }

    /*
     * @see Title#getDefaultText()
     */
    @Override
    protected String getDefaultText() {
        if (direction == Direction.X) {
            return DEFAULT_TEXT_FOR_XAXIS;
        }
        return DEFAULT_TEXT_FOR_YAXIS;
    }

    /*
     * @see Title#isHorizontal()
     */
    @Override
    protected boolean isHorizontal() {
        return axis.isHorizontalAxis();
    }
}
