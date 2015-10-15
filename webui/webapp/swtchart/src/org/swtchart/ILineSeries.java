/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import org.eclipse.swt.graphics.Color;

/**
 * Line series.
 */
public interface ILineSeries extends ISeries {

    /**
     * A plot symbol type.
     */
    public enum PlotSymbolType {

        /** none */
        NONE("None"),

        /** circle */
        CIRCLE("Circle"),

        /** square */
        SQUARE("Square"),

        /** diamond */
        DIAMOND("Diamond"),

        /** triangle */
        TRIANGLE("Triangle"),

        /** inverted triangle */
        INVERTED_TRIANGLE("Inverted Triangle"),

        /** cross */
        CROSS("Cross"),

        /** plus */
        PLUS("Plus");

        /** the label for plot symbol */
        public final String label;

        /**
         * Constructor.
         * 
         * @param label
         *            plot symbol label
         */
        private PlotSymbolType(String label) {
            this.label = label;
        }
    }

    /**
     * Gets the symbol type.
     * 
     * @return the symbol type
     */
    PlotSymbolType getSymbolType();

    /**
     * Sets the symbol type. If null is given, default type
     * <tt>PlotSymbolType.CIRCLE</tt> will be set.
     * 
     * @param type
     *            the symbol type
     */
    void setSymbolType(PlotSymbolType type);

    /**
     * Gets the symbol size in pixels.
     * 
     * @return the symbol size
     */
    int getSymbolSize();

    /**
     * Sets the symbol size in pixels. The default size is 4.
     * 
     * @param size
     *            the symbol size
     */
    void setSymbolSize(int size);

    /**
     * Gets the symbol color.
     * 
     * @return the symbol color
     */
    Color getSymbolColor();

    /**
     * Sets the symbol color. If null is given, default color will be set.
     * 
     * @param color
     *            the symbol color
     */
    void setSymbolColor(Color color);

    /**
     * Gets the symbol colors.
     * 
     * @return the symbol colors, or <tt>null</tt> if no symbol colors are set.
     */
    Color[] getSymbolColors();

    /**
     * Sets the symbol colors. Typically, the number of symbol colors is the
     * same as the number of plots. If the number of symbol colors is less than
     * the number of plots, the rest of plots will have the common color which
     * is set with <tt>setSymbolColor(Color)</tt>.
     * 
     * <p>
     * By default, <tt>null</tt> is set.
     * 
     * @param colors
     *            the symbol colors. If <tt>null</tt> is given, the color which
     *            is set with <tt>setSymbolColor(Color)</tt> will be commonly
     *            used for all plots.
     */
    void setSymbolColors(Color[] colors);

    /**
     * Gets line style.
     * 
     * @return line style.
     */
    LineStyle getLineStyle();

    /**
     * Sets line style. If null is given, default line style will be set.
     * 
     * @param style
     *            line style
     */
    void setLineStyle(LineStyle style);

    /**
     * Gets the line color.
     * 
     * @return the line color
     */
    Color getLineColor();

    /**
     * Sets line color. If null is given, default color will be set.
     * 
     * @param color
     *            the line color
     */
    void setLineColor(Color color);

    /**
     * Gets the line width.
     * 
     * @return the line width
     */
    int getLineWidth();

    /**
     * Sets the width of line connecting data points and also line drawing
     * symbol if applicable (i.e. <tt>PlotSymbolType.CROSS</tt> or
     * <tt>PlotSymbolType.PLUS</tt>). The default width is 1.
     * 
     * @param width
     *            the line width
     */
    void setLineWidth(int width);

    /**
     * Enables the area chart.
     * 
     * @param enabled
     *            true if enabling area chart
     */
    void enableArea(boolean enabled);

    /**
     * Gets the state indicating if area chart is enabled.
     * 
     * @return true if area chart is enabled
     */
    boolean isAreaEnabled();

    /**
     * Enables the step chart.
     * 
     * @param enabled
     *            true if enabling step chart
     */
    void enableStep(boolean enabled);

    /**
     * Gets the state indicating if step chart is enabled.
     * 
     * @return true if step chart is enabled
     */
    boolean isStepEnabled();
}
