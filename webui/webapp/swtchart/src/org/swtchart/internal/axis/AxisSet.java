/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.axis;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.Set;

import org.eclipse.swt.SWT;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.IAxisSet;
import org.swtchart.ISeries;
import org.swtchart.IAxis.Direction;
import org.swtchart.internal.ChartLayout;
import org.swtchart.internal.ChartLayoutData;
import org.swtchart.internal.Legend;
import org.swtchart.internal.Title;
import org.swtchart.internal.series.SeriesSet;

/**
 * An axis container. By default, axis set has X Axis and Y axis with axis id 0.
 */
public class AxisSet implements IAxisSet {

    /** the set of X axes */
    private HashMap<Integer, Axis> xAxisMap;

    /** the set of Y axes */
    private HashMap<Integer, Axis> yAxisMap;

    /** the chart */
    private Chart chart;

    /**
     * Constructor.
     * 
     * @param chart
     *            the chart
     */
    public AxisSet(Chart chart) {
        this.chart = chart;

        xAxisMap = new HashMap<Integer, Axis>();
        yAxisMap = new HashMap<Integer, Axis>();

        // add default axes
        Axis xAxis = new Axis(0, Direction.X, chart);
        Axis yAxis = new Axis(0, Direction.Y, chart);
        xAxisMap.put(0, xAxis);
        yAxisMap.put(0, yAxis);
    }

    /**
     * Gets the axis map for given direction.
     * 
     * @param direction
     *            the direction
     * @return the axis map
     */
    private HashMap<Integer, Axis> getAxisMap(Direction direction) {
        if (direction == Direction.X) {
            return xAxisMap;
        }
        return yAxisMap;
    }

    /*
     * @see IAxisSet#createXAxis()
     */
    public int createXAxis() {
        return createAxis(Direction.X);
    }

    /*
     * @see IAxisSet#createYAxis()
     */
    public int createYAxis() {
        return createAxis(Direction.Y);
    }

    /**
     * Creates the axis for given direction.
     * 
     * @param direction
     *            the direction of axis
     * @return the created axis id
     */
    private int createAxis(Direction direction) {
        int id = getUniqueId(direction);
        Axis axis = new Axis(id, direction, chart);
        getAxisMap(direction).put(id, axis);
        chart.updateLayout();

        SeriesSet series = (SeriesSet) chart.getSeriesSet();
        if (series != null) {
            series.compressAllSeries();
        }
        return id;
    }

    /**
     * Gets a unique axis id.
     * 
     * @param direction
     *            the axis direction
     * @return a unique axis id
     */
    private int getUniqueId(Direction direction) {
        Set<Integer> keySet = getAxisMap(direction).keySet();

        int i = 0;
        while (keySet.contains(i)) {
            i++;
        }
        return i;
    }

    /*
     * @see IAxisSet#getXAxis(int)
     */
    public IAxis getXAxis(int id) {
        return getAxis(id, Direction.X);
    }

    /*
     * @see IAxisSet#getYAxis(int)
     */
    public IAxis getYAxis(int id) {
        return getAxis(id, Direction.Y);
    }

    /**
     * Gets the axis with axis id for given direction.
     * 
     * @param id
     *            the axis id
     * @param direction
     *            the direction
     * @return the axis
     */
    private IAxis getAxis(int id, Direction direction) {
        return getAxisMap(direction).get(id);
    }

    /*
     * @see IAxisSet#getXAxes()
     */
    public IAxis[] getXAxes() {
        return xAxisMap.values().toArray(new IAxis[0]);
    }

    /*
     * @see IAxisSet#getYAxes()
     */
    public IAxis[] getYAxes() {
        return yAxisMap.values().toArray(new IAxis[0]);
    }

    /*
     * @see IAxisSet#getAxes()
     */
    public IAxis[] getAxes() {
        Collection<Axis> axes = new ArrayList<Axis>();
        axes.addAll(xAxisMap.values());
        axes.addAll(yAxisMap.values());
        return axes.toArray(new Axis[0]);
    }

    /*
     * @see IAxisSet#getXAxisIds()
     */
    public int[] getXAxisIds() {
        return getAxisIds(Direction.X);
    }

    /*
     * @see IAxisSet#getYAxisIds()
     */
    public int[] getYAxisIds() {
        return getAxisIds(Direction.Y);
    }

    /**
     * Gets the axis ids for given direction.
     * 
     * @param direction
     *            the direction
     * @return the axis ids
     */
    private int[] getAxisIds(Direction direction) {
        Integer[] array = getAxisMap(direction).keySet()
                .toArray(new Integer[0]);

        int[] ids = new int[array.length];
        for (int i = 0; i < ids.length; i++) {
            ids[i] = array[i];
        }
        Arrays.sort(ids);

        return ids;
    }

    /*
     * @see IAxisSet#deleteXAxis(int)
     */
    public void deleteXAxis(int id) {
        deleteAxis(id, Direction.X);
    }

    /*
     * @see IAxisSet#deleteYAxis(int)
     */
    public void deleteYAxis(int id) {
        deleteAxis(id, Direction.Y);
    }

    /**
     * Deletes the axis with the axis id for given direction.
     * 
     * @param id
     *            the axis id
     * @param direction
     *            the direction
     */
    private void deleteAxis(int id, Direction direction) {
        if (id == 0) {
            SWT.error(SWT.ERROR_CANNOT_BE_ZERO);
        }

        if (getAxisMap(direction).get(id) == null) {
            throw new IllegalArgumentException("Given axis id doesn't exist");
        }

        ((Axis) getAxis(id, direction)).dispose();
        getAxisMap(direction).remove(id);

        for (ISeries series : chart.getSeriesSet().getSeries()) {
            if (direction == Direction.X) {
                if (series.getXAxisId() == id) {
                    series.setXAxisId(0);
                }
            } else {
                if (series.getYAxisId() == id) {
                    series.setYAxisId(0);
                }
            }
        }
        chart.updateLayout();
    }

    /*
     * @see IAxisSet#adjustRange()
     */
    public void adjustRange() {
        for (IAxis axis : getAxes()) {
            ((Axis) axis).adjustRange(false);
        }
        chart.updateLayout();
    }

    /*
     * @see IAxisSet#zoomIn()
     */
    public void zoomIn() {
        for (IAxis axis : getAxes()) {
            axis.zoomIn();
        }
    }

    /*
     * @see IAxisSet#zoomOut()
     */
    public void zoomOut() {
        for (IAxis axis : getAxes()) {
            axis.zoomOut();
        }
    }

    /**
     * Updates the layout data.
     */
    public void updateLayoutData() {
        IAxis[] horizontalAxes;
        IAxis[] verticalAxes;
        if (chart.getOrientation() == SWT.HORIZONTAL) {
            horizontalAxes = getXAxes();
            verticalAxes = getYAxes();
        } else {
            horizontalAxes = getYAxes();
            verticalAxes = getXAxes();
        }

        updateAxesLayoutData(horizontalAxes);
        updateVerticalTick(horizontalAxes, verticalAxes);
        updateAxesLayoutData(verticalAxes);
        updateHorizontalTick(horizontalAxes, verticalAxes);
    }

    /**
     * Updates the layout data
     * 
     * @param axes
     *            The axes
     */
    private void updateAxesLayoutData(IAxis[] axes) {
        for (IAxis axis : axes) {
            ((Axis) axis).updateLayoutData();
        }
    }

    /**
     * Updates the horizontal tick.
     * 
     * @param horizontalAxes
     *            the horizontal axes
     * @param verticalAxes
     *            the vertical axes
     */
    private void updateHorizontalTick(IAxis[] horizontalAxes,
            IAxis[] verticalAxes) {
        int legendPosition = ((Legend) chart.getLegend()).getPosition();
        int legendWidth = ((ChartLayoutData) ((Legend) chart.getLegend())
                .getLayoutData()).widthHint;
        int axesWidth = 0;

        for (IAxis axis : verticalAxes) {
            axesWidth += ((ChartLayoutData) ((Title) ((Axis) axis).getTitle())
                    .getLayoutData()).widthHint
                    + ((Axis) axis).getTick().getAxisTickLabels()
                            .getLayoutData().widthHint
                    + ((Axis) axis).getTick().getAxisTickMarks()
                            .getLayoutData().widthHint;
        }

        int axisWidth = chart.getSize().x
                - axesWidth
                - ChartLayout.MARGIN
                * 2
                - (legendPosition == SWT.LEFT || legendPosition == SWT.RIGHT ? legendWidth
                        + (legendWidth == 0 ? 0 : ChartLayout.PADDING)
                        : 0);

        for (IAxis axis : horizontalAxes) {
            ((Axis) axis).getTick().updateTick(axisWidth);
        }
    }

    /**
     * Updates the vertical tick.
     * 
     * @param horizontalAxes
     *            the horizontal axes
     * @param verticalAxes
     *            the vertical axes
     */
    private void updateVerticalTick(IAxis[] horizontalAxes, IAxis[] verticalAxes) {
        int legendPosition = ((Legend) chart.getLegend()).getPosition();
        int legendHeight = ((ChartLayoutData) ((Legend) chart.getLegend())
                .getLayoutData()).heightHint;
        int titleHeight = ((ChartLayoutData) ((Title) chart.getTitle())
                .getLayoutData()).heightHint;
        int axesHeight = 0;

        for (IAxis axis : horizontalAxes) {
            axesHeight += ((ChartLayoutData) ((Title) ((Axis) axis).getTitle())
                    .getLayoutData()).heightHint
                    + ((Axis) axis).getTick().getAxisTickLabels()
                            .getLayoutData().heightHint
                    + ((Axis) axis).getTick().getAxisTickMarks()
                            .getLayoutData().heightHint;
        }

        int axisHeight = chart.getSize().y
                - titleHeight
                - axesHeight
                - ChartLayout.MARGIN
                * 2
                - (titleHeight == 0 ? 0 : ChartLayout.PADDING)
                - ((legendPosition == SWT.TOP || legendPosition == SWT.BOTTOM) ? legendHeight
                        + (legendHeight == 0 ? 0 : ChartLayout.PADDING)
                        : 0);

        for (IAxis axis : verticalAxes) {
            ((Axis) axis).getTick().updateTick(axisHeight);
        }
    }

    /**
     * Refreshes the cache.
     */
    public void refresh() {
        for (IAxis axis : getAxes()) {
            ((Axis) axis).refresh();
        }
    }

    /**
     * Disposes the resources.
     */
    public void dispose() {
        for (IAxis axis : getAxes()) {
            ((Axis) axis).dispose();
        }
    }
}
