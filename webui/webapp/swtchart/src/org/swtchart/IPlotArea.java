/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

/**
 * The plot area.
 */
public interface IPlotArea {

    /**
     * Adds the custom paint listener.
     *
     * @param listener
     *            the custom paint listener
     */
    public void addCustomPaintListener(ICustomPaintListener listener);

    /**
     * Removes the custom paint listener
     *
     * @param listener
     *            the custom paint listener
     */
    public void removeCustomPaintListener(ICustomPaintListener listener);
}
