/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

/**
 * An axis container. By default, axis set has X Axis and Y axis with axis id 0.
 */
public interface IAxisSet {

    /**
     * Creates the X axis. This method is used for multiple axes chart.
     * 
     * @return the axis id
     */
    int createXAxis();

    /**
     * Creates the Y axis. This method is used for multiple axes chart.
     * 
     * @return the axis id
     */
    int createYAxis();

    /**
     * Gets the X axis for the given id.
     * 
     * @param id
     *            the axis id
     * @return the X axis for the given axis id, or null if there is no
     *         corresponding axis.
     */
    IAxis getXAxis(int id);

    /**
     * Gets the Y axis for given index.
     * 
     * @param id
     *            the axis id
     * @return the Y axis for the given axis id, or null if there is no
     *         corresponding axis.
     */
    IAxis getYAxis(int id);

    /**
     * Gets the array of X axes.
     * 
     * @return the array of X axes
     */
    IAxis[] getXAxes();

    /**
     * Gets the array of Y axes.
     * 
     * @return the array of Y axes
     */
    IAxis[] getYAxes();

    /**
     * Gets the array of all axes.
     * 
     * @return the array of all axes
     */
    IAxis[] getAxes();

    /**
     * Gets the array of X axis ids.
     * 
     * @return the array of X axis ids
     */
    int[] getXAxisIds();

    /**
     * Gets the array of Y axis ids.
     * 
     * @return the array of Y axis ids
     */
    int[] getYAxisIds();

    /**
     * Deletes the X Axis for given axis id. The series on the deleted axis will
     * be moved onto the axis id '0'. The axis whose id is '0' cannot be
     * removed.
     * 
     * @param id
     *            the axis id
     * @throws IllegalArgumentException
     *             if the given axis id is '0', or if there is no axis for the
     *             given id.
     */
    void deleteXAxis(int id);

    /**
     * Deletes the Y Axis for given id. The series on the deleted axis will be
     * moved onto the axis id '0'. The axis whose id is '0' cannot be removed.
     * 
     * @param id
     *            the axis id
     * @throws IllegalArgumentException
     *             if the given axis id is '0', or if there is no axis for the
     *             given id.
     */
    void deleteYAxis(int id);

    /**
     * Adjusts the axis range of all axes.
     */
    void adjustRange();

    /**
     * Zooms in all axes.
     */
    void zoomIn();

    /**
     * Zooms out all axes.
     */
    void zoomOut();
}