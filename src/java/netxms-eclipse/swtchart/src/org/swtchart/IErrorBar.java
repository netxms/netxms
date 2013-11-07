/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import org.eclipse.swt.graphics.Color;

/**
 * An error bar.
 */
public interface IErrorBar {

    /**
     * The error bar type.
     */
    public enum ErrorBarType {

        /** the error bar in both positive and negative directions */
        BOTH("Both"),

        /** the error bar in positive direction */
        PLUS("Plus"),

        /** the error bar in negative direction */
        MINUS("Minus");

        /** the label for error bar type */
        public final String label;

        /**
         * The constructor.
         * 
         * @param label
         *            error bar type label
         */
        private ErrorBarType(String label) {
            this.label = label;
        }
    }

    /**
     * Gets the error type.
     * 
     * @return the error type
     */
    ErrorBarType getType();

    /**
     * Sets the error type.
     * 
     * @param type
     *            the error type
     */
    void setType(ErrorBarType type);

    /**
     * Gets the error bar color. The default color is dark gray.
     * 
     * @return the error bar color
     */
    Color getColor();

    /**
     * Sets the error bar color. If <tt>null</tt> is given, default color will
     * be set.
     * 
     * @param color
     *            the error bar color
     */
    void setColor(Color color);

    /**
     * Gets the line width to draw error bar.
     * 
     * @return the line width to draw error bar
     */
    int getLineWidth();

    /**
     * Sets the line width to draw error bar. The default line width is 1.
     * 
     * @param width
     *            line width to draw error bar
     */
    void setLineWidth(int width);

    /**
     * Gets the error.
     * 
     * @return the error
     */
    double getError();

    /**
     * Sets the error.
     * <p>
     * If errors have been set with {@link #getPlusErrors()} or
     * {@link #getMinusErrors()}, the value set with this method won't be used.
     * 
     * @param error
     *            the error
     */
    void setError(double error);

    /**
     * Gets the plus errors.
     * 
     * @return the plus errors, or <tt>null</tt> if errors are not set.
     */
    double[] getPlusErrors();

    /**
     * Sets the plus errors.
     * 
     * @param errors
     *            the plus errors
     */
    void setPlusErrors(double[] errors);

    /**
     * Gets the minus errors.
     * 
     * @return the minus errors, or <tt>null</tt> if errors are not set.
     */
    double[] getMinusErrors();

    /**
     * Sets the minus errors.
     * 
     * @param errors
     *            the minus errors
     */
    void setMinusErrors(double[] errors);

    /**
     * Sets the visibility state.
     * 
     * @param visible
     *            the visibility state
     */
    void setVisible(boolean visible);

    /**
     * Gets the visibility state.
     * 
     * @return true if error bar is visible
     */
    boolean isVisible();
}
