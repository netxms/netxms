/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;

/**
 * A series label.
 */
public interface ISeriesLabel {

    /**
     * Sets the decimal format {@link java.text.DecimalFormat} or/plus plain string.
     * <p>
     * If formats have been set with <tt>setFormats(String[])</tt>, the format
     * set with this method will be ignored.
     * <p>
     * If null is given, default format "#.###########" will be set.
     * 
     * @param format
     *            the format
     */
    void setFormat(String format);

    /**
     * Gets the format for label.
     * 
     * @return the format
     */
    String getFormat();

    /**
     * Sets the formats for all data points. If null is given, formats will be
     * cleared, and the format set with <tt>setFormat(String)</tt> will be used
     * instead.
     * 
     * @param formats
     *            the formats
     */
    void setFormats(String[] formats);

    /**
     * Gets the formats for all data points.
     * 
     * @return the formats
     */
    String[] getFormats();

    /**
     * Sets the label color. If null is given, default color will be set.
     * 
     * @param color
     *            the label color
     */
    void setForeground(Color color);

    /**
     * Gets the label color.
     * 
     * @return the label color
     */
    Color getForeground();

    /**
     * Sets the label font.
     * 
     * @param font
     *            the label font
     */
    void setFont(Font font);

    /**
     * Gets the label font.
     * 
     * @return the label font
     */
    Font getFont();

    /**
     * Sets the label visibility state.
     * 
     * @param visible
     *            the label visibility state
     */
    void setVisible(boolean visible);

    /**
     * Gets the label visibility state.
     * 
     * @return true if label is visible
     */
    boolean isVisible();

}
