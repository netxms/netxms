/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import org.eclipse.swt.graphics.Color;

/**
 * A grid.
 */
public interface IGrid {

    /**
     * Gets the foreground color.
     * 
     * @return the foreground color
     */
    Color getForeground();

    /**
     * Sets the foreground color.
     * 
     * @param color
     *            the foreground color
     */
    void setForeground(Color color);

    /**
     * Gets the line style.
     * 
     * @return the line style.
     */
    LineStyle getStyle();

    /**
     * Sets the line style.
     * 
     * @param style
     *            the line style
     */
    void setStyle(LineStyle style);
}