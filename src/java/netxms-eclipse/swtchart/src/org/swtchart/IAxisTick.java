/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved.
 *
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import java.text.Format;

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Rectangle;

/**
 * An axis tick.
 */
public interface IAxisTick {

    /** the minimum grid step hint */
    public static final double MIN_GRID_STEP_HINT = 16;

    /**
     * Sets the foreground color of axis tick.
     *
     * @param color
     *            the foreground color of axis tick
     */
    public void setForeground(Color color);

    /**
     * Gets the foreground color of axis tick.
     *
     * @return the foreground color of axis tick
     */
    public Color getForeground();

    /**
     * Sets the font for tick labels.
     *
     * @param font
     *            the font for tick labels
     */
    public void setFont(Font font);

    /**
     * Gets the font for tick labels.
     *
     * @return the font for tick labels
     */
    Font getFont();

    /**
     * Gets the state indicating if tick marks are visible.
     *
     * @return true if tick marks are visible
     */
    boolean isVisible();

    /**
     * Sets the state indicating if tick marks are visible.
     *
     * @param isVisible
     *            true to make the tick marks visible
     */
    void setVisible(boolean isVisible);

    /**
     * Gets the tick mark step hint in pixels.
     *
     * @return the tick mark step hint in pixels
     */
    int getTickMarkStepHint();

    /**
     * Sets the tick mark step hint in pixels.
     *
     * @param tickMarkStepHint
     *            the tick mark step hint with pixels (>
     *            IAxisTick.MIN_GRID_STEP_HINT)
     */
    void setTickMarkStepHint(int tickMarkStepHint);

    /**
     * Gets the tick label angle.
     *
     * @return the tick label angle in degree
     */
    int getTickLabelAngle();

    /**
     * Sets the tick label angle.
     *
     * @param angle
     *            the angle in degree between 0 and 90. The default value is 0.
     *            If setting 0, tick labels are horizontally shown. If setting
     *            90, tick labels are vertically shown.
     */
    void setTickLabelAngle(int angle);

    /**
     * Sets the format for axis tick label. <tt>DecimalFormat</tt> and
     * <tt>DateFormat</tt> should be used for <tt>double[]</tt> series and
     * <tt>Date[]</tt> series respectively.
     *
     * @param format
     *            the format
     */
    void setFormat(Format format);

    /**
     * Gets the format for axis tick label.
     *
     * @return the format
     */
    Format getFormat();

    /**
     * Gets the bounds of axis tick.
     * <p>
     * This method is typically used for mouse listener to check whether mouse
     * cursor is on axis tick. Mouse listener can be added to <tt>Chart</tt>.
     *
     * @return the bounds of axis tick.
     */
    Rectangle getBounds();

    /**
     * Gets the tick label values.
     *
     * @return the tick label values
     */
    double[] getTickLabelValues();
}
