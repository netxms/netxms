/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Rectangle;

/**
 * A legend for chart.
 */
public interface ILegend {

    /**
     * Sets legend visible.
     * 
     * @param visible
     *            the visibility state
     */
    void setVisible(boolean visible);

    /**
     * Gets the visibility state.
     * 
     * @return true if legend is visible
     */
    boolean isVisible();

    /**
     * Sets the background color of legend.
     * 
     * @param color
     *            the background color
     */
    void setBackground(Color color);

    /**
     * Gets the background color of legend.
     * 
     * @return background color of legend.
     */
    Color getBackground();

    /**
     * Sets the foreground color of legend.
     * 
     * @param color
     *            the foreground color
     */
    void setForeground(Color color);

    /**
     * Gets the foreground color of legend.
     * 
     * @return foreground color of legend.
     */
    Color getForeground();

    /**
     * Gets the font.
     * 
     * @return the font
     */
    Font getFont();

    /**
     * Sets the font.
     * 
     * @param font
     *            the font
     */
    void setFont(Font font);

    /**
     * Gets the position of legend.
     * 
     * @return the position of legend.
     */
    int getPosition();

    /**
     * Sets the position of legend. If the position is <tt>SWT.LEFT</tt> or
     * <tt>SWT.RIGHT</tt>, the orientation of series on legend will be vertical.
     * If the position is <tt>SWT.TOP</tt> or <tt>SWT.BOTTOM</tt>, the
     * orientation will be horizontal.
     * 
     * @param position
     *            the position of legend that can be <tt>SWT.LEFT</tt>,
     *            <tt>SWT.RIGHT</tt>, <tt>SWT.TOP</tt> or <tt>SWT.BOTTOM</tt>.
     */
    void setPosition(int position);

    /**
     * Gets the rectangle associated with the given series id on legend. This
     * method is typically used for mouse listener to check whether mouse cursor
     * is on legend for a certain series.
     * <p>
     * Mouse listener can be added by casting <tt>ILegend</tt> to
     * <tt>Control</tt>.
     * 
     * <pre>
     * Control legend = (Control) chart.getLegend();
     * legend.addMouseListener(...);
     * </pre>
     * 
     * @param seriesId
     *            the series id
     * @return the rectangle associated with the given series id in pixels.
     */
    Rectangle getBounds(String seriesId);
    
    /**
    * @return
    */
   public boolean isExtended();
    
   /**
    * @param extended
    */
   public void setExtended(boolean extended);
}
