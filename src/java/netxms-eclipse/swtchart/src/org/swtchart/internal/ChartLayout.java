/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Layout;
import org.swtchart.Chart;
import org.swtchart.IAxis;
import org.swtchart.IAxisSet;
import org.swtchart.IAxis.Position;
import org.swtchart.internal.axis.Axis;
import org.swtchart.internal.axis.AxisTickLabels;
import org.swtchart.internal.axis.AxisTickMarks;
import org.swtchart.internal.axis.AxisTitle;

/**
 * Manages the layout on plot chart panel.
 */
public class ChartLayout extends Layout
{

    /** the title height */
    private int titleHeight;

    /** the title width */
    private int titleWidth;

    /** the legend width */
    private int legendWidth;

    /** the legend height */
    private int legendHeight;

    /** the bottom axis height */
    private int bottomAxisHeight;

    /** the top axis height */
    private int topAxisHeight;

    /** the left axis width */
    private int leftAxisWidth;

    /** the right axis width */
    private int rightAxisWidth;

    /** the chart title */
    private ChartTitle title;

    /** the legend */
    private Legend legend;

    /** the plot area */
    private PlotArea plot;

    /** the axes */
    private List<Axis> axes;

    /** the offset for bottom axis */
    private int bottomAxisOffset = 0;

    /** the offset for top axis */
    private int topAxisOffset = 0;

    /** the offset for left axis */
    private int leftAxisOffset = 0;

    /** the offset for right axis */
    private int rightAxisOffset = 0;

    /** the margin */
    public static final int MARGIN = 5;

    /** the padding */
    public static final int PADDING = 5;

    /**
     * Axis layout data.
     */
    private static class AxisLayoutData {

        /** the axis tick marks */
        public AxisTickMarks axisTickMarks;

        /** the axis tick labels */
        public AxisTickLabels axisTickLabels;

        /** the axis title */
        public AxisTitle axisTitle;

        /** the axis title layout data */
        public ChartLayoutData titleLayoutdata;

        /** the tick label layout data */
        public ChartLayoutData tickLabelsLayoutdata;

        /** the tick marks layout data */
        public ChartLayoutData tickMarksLayoutdata;

        /**
         * Constructor.
         * 
         * @param axis
         *            the axis
         */
        public AxisLayoutData(Axis axis) {
            axisTickMarks = axis.getTick().getAxisTickMarks();
            axisTickLabels = axis.getTick().getAxisTickLabels();
            axisTitle = (AxisTitle) axis.getTitle();

            titleLayoutdata = (ChartLayoutData) axisTitle.getLayoutData();
            tickLabelsLayoutdata = axisTickLabels.getLayoutData();
            tickMarksLayoutdata = axisTickMarks.getLayoutData();
        }
    }

    /**
     * Constructor.
     */
    public ChartLayout() {
        initWidgetSizeVariables();
        axes = new ArrayList<Axis>();
    }

    /**
     * Initializes the size variables of widgets.
     */
    private void initWidgetSizeVariables() {
        titleHeight = 0;
        titleWidth = 0;
        bottomAxisHeight = 0;
        topAxisHeight = 0;
        leftAxisWidth = 0;
        rightAxisWidth = 0;
        legendWidth = 0;
        legendHeight = 0;
    }

    /*
     * @see Layout#computeSize(Composite , int, int, boolean)
     */
    @Override
    protected Point computeSize(Composite composite, int wHint, int hHint,
            boolean flushCache) {
        return new Point(wHint, hHint);
    }

    /*
     * @see Layout#layout(Composite, boolean)
     */
    @Override
    protected void layout(Composite composite, boolean flushCache) {
        if (!parseControls(composite)) {
            return;
        }

        initWidgetSizeVariables();
        computeControlSize();

        Rectangle r = composite.getClientArea();
        layoutTitle(r);
        layoutLegend(r);
        layoutPlot(r);
        layoutAxes(r);
    }

    /**
     * Parses the controls on given composite.
     * 
     * @param composite
     *            the composite
     * @return true if all children found
     */
    private boolean parseControls(Composite composite) {
        Control[] children = composite.getChildren();
        axes.clear();
        for (Control child : children) {
            if (child instanceof Legend) {
                legend = (Legend) child;
            } else if (child instanceof ChartTitle) {
                title = (ChartTitle) child;
            } else if (child instanceof PlotArea) {
                plot = (PlotArea) child;
            }
        }

        if (composite instanceof Chart) {
            IAxisSet axisSet = ((Chart) composite).getAxisSet();
            if (axisSet != null) {
                IAxis[] axisArray = axisSet.getAxes();
                for (IAxis axis : axisArray) {
                    axes.add((Axis) axis);
                }
            }
        }

        if (title == null || legend == null || plot == null || axes.size() < 2) {
            // the initialization of chart is not completed yet
            return false;
        }
        return true;
    }

    /**
     * Computes the size of child controls.
     */
    private void computeControlSize() {
        titleWidth = ((ChartLayoutData) title.getLayoutData()).widthHint;
        titleHeight = ((ChartLayoutData) title.getLayoutData()).heightHint;
        legendWidth = ((ChartLayoutData) legend.getLayoutData()).widthHint;
        legendHeight = ((ChartLayoutData) legend.getLayoutData()).heightHint;

        for (Axis axis : axes) {
            AxisLayoutData layoutData = new AxisLayoutData(axis);
            if (layoutData.titleLayoutdata == null
                    || layoutData.tickLabelsLayoutdata == null
                    || layoutData.tickMarksLayoutdata == null) {
                continue;
            }

            Position position = axis.getPosition();
            if (position == Position.Primary && axis.isHorizontalAxis()) {
                bottomAxisHeight += layoutData.titleLayoutdata.heightHint
                        + layoutData.tickLabelsLayoutdata.heightHint
                        + layoutData.tickMarksLayoutdata.heightHint;
            } else if (position == Position.Secondary
                    && axis.isHorizontalAxis()) {
                topAxisHeight += layoutData.titleLayoutdata.heightHint
                        + layoutData.tickLabelsLayoutdata.heightHint
                        + layoutData.tickMarksLayoutdata.heightHint;
            } else if (position == Position.Primary && !axis.isHorizontalAxis()) {
                leftAxisWidth += layoutData.titleLayoutdata.widthHint
                        + layoutData.tickLabelsLayoutdata.widthHint
                        + layoutData.tickMarksLayoutdata.widthHint;
            } else if (position == Position.Secondary
                    && !axis.isHorizontalAxis()) {
                rightAxisWidth += layoutData.titleLayoutdata.widthHint
                        + layoutData.tickLabelsLayoutdata.widthHint
                        + layoutData.tickMarksLayoutdata.widthHint;
            }
        }
    }

    /**
     * Layouts the title.
     * 
     * @param r
     *            the rectangle to layout
     */
    private void layoutTitle(Rectangle r) {
        int x = (int) ((r.width - titleWidth) / 2d);
        int y = MARGIN;
        int width = titleWidth;
        int height = titleHeight;

        title.setBounds(x, y, width, height);
    }

    /**
     * Layouts the legend.
     * 
     * @param r
     *            the rectangle to layout
     */
    private void layoutLegend(Rectangle r) {
        int legendPosition = legend.getPosition();

        int tHeight = titleHeight + ((titleHeight == 0) ? 0 : PADDING);
        int x;
        int y;
        if (legendPosition == SWT.RIGHT) {
            x = r.width - legendWidth - MARGIN;
            y = (tHeight + r.height - legendHeight) / 2;
        } else if (legendPosition == SWT.LEFT) {
            x = MARGIN;
            y = (tHeight + r.height - legendHeight) / 2;
        } else if (legendPosition == SWT.TOP) {
            x = (r.width - legendWidth) / 2;
            y = tHeight + MARGIN;
        } else if (legendPosition == SWT.BOTTOM) {
            x = (r.width - legendWidth) / 2;
            y = r.height - legendHeight - MARGIN;
        } else {
            throw new IllegalStateException();
        }
        int width = legendWidth;
        int height = legendHeight;

        if (y < tHeight) {
            y = tHeight;
        }

        legend.setBounds(x, y, width, height);
    }

    /**
     * Layouts the plot.
     * 
     * @param r
     *            the rectangle to layout
     */
    private void layoutPlot(Rectangle r) {
        int legendPosition = legend.getPosition();

        int x = leftAxisWidth
                + MARGIN
                + (legendPosition == SWT.LEFT ? legendWidth
                        + (legendWidth == 0 ? 0 : PADDING) : 0);
        int y = titleHeight
                + topAxisHeight
                + MARGIN
                + (titleHeight == 0 ? 0 : PADDING)
                + (legendPosition == SWT.TOP ? legendHeight
                        + (legendHeight == 0 ? 0 : PADDING) : 0);
        int width = r.width
                - leftAxisWidth
                - rightAxisWidth
                - (legendPosition == SWT.LEFT || legendPosition == SWT.RIGHT ? legendWidth
                        + (legendWidth == 0 ? 0 : PADDING)
                        : 0) - MARGIN * 2;
        int height = r.height
                - bottomAxisHeight
                - topAxisHeight
                - titleHeight
                - MARGIN
                * 2
                - (titleHeight == 0 ? 0 : PADDING)
                - (legendPosition == SWT.TOP || legendPosition == SWT.BOTTOM ? legendHeight
                        + (legendHeight == 0 ? 0 : PADDING)
                        : 0);

        plot.setBounds(x, y, width, height);
    }

    /**
     * Layouts the axes.
     * 
     * @param r
     *            the rectangle to layout
     */
    private void layoutAxes(Rectangle r) {
        bottomAxisOffset = 0;
        topAxisOffset = 0;
        leftAxisOffset = 0;
        rightAxisOffset = 0;

        for (Axis axis : axes) {
            AxisLayoutData layoutData = new AxisLayoutData(axis);
            if (layoutData.titleLayoutdata == null
                    || layoutData.tickLabelsLayoutdata == null
                    || layoutData.tickMarksLayoutdata == null) {
                continue;
            }
            Position position = axis.getPosition();
            if (position == Position.Primary && axis.isHorizontalAxis()) {
                layoutBottomAxis(r, layoutData);
            } else if (position == Position.Secondary
                    && axis.isHorizontalAxis()) {
                layoutTopAxis(r, layoutData);
            } else if (position == Position.Primary && !axis.isHorizontalAxis()) {
                layoutLeftAxis(r, layoutData);
            } else if (position == Position.Secondary
                    && !axis.isHorizontalAxis()) {
                layoutRightAxis(r, layoutData);
            }
        }
    }

    /**
     * Layouts the bottom axis.
     * 
     * @param r
     *            the rectangle
     * @param layoutData
     *            the layout data
     */
    private void layoutBottomAxis(Rectangle r, AxisLayoutData layoutData) {
        int legendPosition = legend.getPosition();

        int width = r.width
                - leftAxisWidth
                - rightAxisWidth
                - (legendPosition == SWT.LEFT || legendPosition == SWT.RIGHT ? legendWidth
                        + (legendWidth == 0 ? 0 : PADDING)
                        : 0) - MARGIN * 2;
        int height = layoutData.titleLayoutdata.heightHint;
        int x = leftAxisWidth
                + MARGIN
                + (legendPosition == SWT.LEFT ? legendWidth
                        + (legendWidth == 0 ? 0 : PADDING) : 0);
        int y = r.height
                - height
                - bottomAxisOffset
                - MARGIN
                - (legendPosition == SWT.BOTTOM ? legendHeight
                        + (legendHeight == 0 ? 0 : PADDING) : 0);

        bottomAxisOffset += height;
        if (y - layoutData.tickLabelsLayoutdata.heightHint
                - layoutData.tickMarksLayoutdata.heightHint < titleHeight
                + (titleHeight == 0 ? 0 : PADDING)) {
            y = titleHeight + (titleHeight == 0 ? 0 : PADDING)
                    + layoutData.tickLabelsLayoutdata.heightHint
                    + layoutData.tickMarksLayoutdata.heightHint;
        }
        width = width > 0 ? width : 0;
        layoutData.axisTitle.setBounds(x, y, width, height);

        height = layoutData.tickLabelsLayoutdata.heightHint;
        y -= height;
        bottomAxisOffset += height;
        layoutData.axisTickLabels.setBounds(0, y, r.width, height);

        height = layoutData.tickMarksLayoutdata.heightHint;
        y -= height;
        bottomAxisOffset += height;
        layoutData.axisTickMarks.setBounds(x, y, width, height);
    }

    /**
     * Layouts the top axis.
     * 
     * @param r
     *            the rectangle
     * @param layoutData
     *            the layout data
     */
    private void layoutTopAxis(Rectangle r, AxisLayoutData layoutData) {
        int legendPosition = legend.getPosition();

        int width = r.width
                - leftAxisWidth
                - rightAxisWidth
                - (legendPosition == SWT.LEFT || legendPosition == SWT.RIGHT ? legendWidth
                        + (legendWidth == 0 ? 0 : PADDING)
                        : 0) - MARGIN * 2;
        int height = layoutData.titleLayoutdata.heightHint;
        int x = leftAxisWidth
                + MARGIN
                + (legendPosition == SWT.LEFT ? legendWidth
                        + (legendWidth == 0 ? 0 : PADDING) : 0);
        int y = titleHeight + topAxisOffset + MARGIN
                + ((titleHeight == 0) ? 0 : PADDING);

        topAxisOffset += height;
        width = width > 0 ? width : 0;
        layoutData.axisTitle.setBounds(x, y, width, height);

        y += height;
        height = layoutData.tickLabelsLayoutdata.heightHint;
        topAxisOffset += height;
        layoutData.axisTickLabels.setBounds(0, y, r.width, height);

        y += height;
        height = layoutData.tickMarksLayoutdata.heightHint;
        topAxisOffset += height;
        layoutData.axisTickMarks.setBounds(x, y, width, height);
    }

    /**
     * Layouts the left axis.
     * 
     * @param r
     *            the rectangle
     * @param layoutData
     *            the layout data
     */
    private void layoutLeftAxis(Rectangle r, AxisLayoutData layoutData) {
        int legendPosition = legend.getPosition();

        int yAxisMargin = Axis.MARGIN + AxisTickMarks.TICK_LENGTH;

        int width = layoutData.titleLayoutdata.widthHint;
        int height = r.height
                - bottomAxisHeight
                - topAxisHeight
                - titleHeight
                - MARGIN
                * 2
                - ((titleHeight == 0) ? 0 : PADDING)
                - (legendPosition == SWT.TOP || legendPosition == SWT.BOTTOM ? legendHeight
                        + (legendHeight == 0 ? 0 : PADDING)
                        : 0);
        int x = MARGIN
                + leftAxisOffset
                + (legendPosition == SWT.LEFT ? legendWidth
                        + (legendWidth == 0 ? 0 : PADDING) : 0);
        int y = titleHeight
                + topAxisHeight
                + MARGIN
                + ((titleHeight == 0) ? 0 : PADDING)
                + (legendPosition == SWT.TOP ? legendHeight
                        + (legendHeight == 0 ? 0 : PADDING) : 0);

        leftAxisOffset += width;
        height = height > 0 ? height : 0;
        layoutData.axisTitle.setBounds(x, y, width, height);

        x += width;
        width = layoutData.tickLabelsLayoutdata.widthHint;
        leftAxisOffset += width;
        layoutData.axisTickLabels.setBounds(x, y - yAxisMargin, width, height
                + yAxisMargin * 2);

        x += width;
        width = layoutData.tickMarksLayoutdata.widthHint;
        leftAxisOffset += width;
        layoutData.axisTickMarks.setBounds(x, y, width, height);
    }

    /**
     * Layouts the right axis.
     * 
     * @param r
     *            the rectangle
     * @param layoutData
     *            the layout data
     */
    private void layoutRightAxis(Rectangle r, AxisLayoutData layoutData) {
        int legendPosition = legend.getPosition();

        int yAxisMargin = Axis.MARGIN + AxisTickMarks.TICK_LENGTH;

        int width = layoutData.titleLayoutdata.widthHint;
        int height = r.height
                - bottomAxisHeight
                - topAxisHeight
                - titleHeight
                - MARGIN
                * 2
                - ((titleHeight == 0) ? 0 : PADDING)
                - (legendPosition == SWT.TOP || legendPosition == SWT.BOTTOM ? legendHeight
                        + (legendHeight == 0 ? 0 : PADDING)
                        : 0);
        int x = r.width
                - width
                - rightAxisOffset
                - MARGIN
                - (legendPosition == SWT.RIGHT ? legendWidth
                        + (legendWidth == 0 ? 0 : PADDING) : 0);
        int y = titleHeight
                + topAxisHeight
                + MARGIN
                + ((titleHeight == 0) ? 0 : PADDING)
                + (legendPosition == SWT.TOP ? legendHeight
                        + (legendHeight == 0 ? 0 : PADDING) : 0);

        rightAxisOffset += width;
        height = height > 0 ? height : 0;
        layoutData.axisTitle.setBounds(x, y, width, height);

        width = layoutData.tickLabelsLayoutdata.widthHint;
        x -= width;
        rightAxisOffset += width;
        layoutData.axisTickLabels.setBounds(x, y - yAxisMargin, width, height
                + yAxisMargin * 2);

        width = layoutData.tickMarksLayoutdata.widthHint;
        x -= width;
        rightAxisOffset += width;
        layoutData.axisTickMarks.setBounds(x, y, width, height);
    }
}
