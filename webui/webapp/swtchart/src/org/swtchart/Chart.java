/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.swtchart.internal.ChartLayout;
import org.swtchart.internal.ChartLayoutData;
import org.swtchart.internal.ChartTitle;
import org.swtchart.internal.Legend;
import org.swtchart.internal.PlotArea;
import org.swtchart.internal.Title;
import org.swtchart.internal.axis.AxisSet;

/**
 * A chart which are composed of title, legend, axes and plot area.
 */
public class Chart extends Canvas implements Listener
{
	private static final long serialVersionUID = 1L;

	/** the title */
    private Title title;

    /** the legend */
    private Legend legend;

    /** the set of axes */
    private AxisSet axisSet;

    /** the plot area */
    private PlotArea plotArea;

    /** the orientation of chart which can be horizontal or vertical */
    private int orientation;

    /** the state indicating if compressing series is enabled */
    private boolean compressEnabled;

    /**
     * Constructor.
     * 
     * @param parent
     *            the parent composite on which chart is placed
     * @param style
     *            the style of widget to construct
     */
    public Chart(Composite parent, int style) {
        super(parent, style);

        orientation = SWT.HORIZONTAL;
        compressEnabled = true;

        parent.layout();

        setLayout(new ChartLayout());

        title = new ChartTitle(this, SWT.NONE);
        title.setLayoutData(new ChartLayoutData(SWT.DEFAULT, 100));
        legend = new Legend(this, SWT.NONE);
        legend.setLayoutData(new ChartLayoutData(200, SWT.DEFAULT));
        plotArea = new PlotArea(this, SWT.NONE);
        axisSet = new AxisSet(this);

        updateLayout();

        addListener(SWT.Resize, this);
    }

    /**
     * Gets the chart title.
     * 
     * @return the chart title
     */
    public ITitle getTitle() {
        return title;
    }

    /**
     * Gets the legend.
     * 
     * @return the legend
     */
    public ILegend getLegend() {
        return legend;
    }

    /**
     * Gets the set of axes.
     * 
     * @return the set of axes
     */
    public IAxisSet getAxisSet() {
        return axisSet;
    }

    /**
     * Gets the plot area.
     * 
     * @return the plot area
     */
    public Canvas getPlotArea() {
        return plotArea;
    }

    /**
     * Gets the set of series.
     * 
     * @return the set of series
     */
    public ISeriesSet getSeriesSet() {
        return plotArea.getSeriesSet();
    }

    /*
     * @see Control#setBackground(Color)
     */
    @Override
    public void setBackground(Color color) {
        super.setBackground(color);

        for (Control child : getChildren()) {
            if (!(child instanceof PlotArea) && !(child instanceof Legend)) {
                child.setBackground(color);
            }
        }
    }

    /**
     * Gets the background color in plot area. This method is identical with
     * <tt>getPlotArea().getBackground()</tt>.
     * 
     * @return the background color in plot area
     */
    public Color getBackgroundInPlotArea() {
        return plotArea.getBackground();
    }

    /**
     * Sets the background color in plot area.
     * 
     * @param color
     *            the background color in plot area. If <tt>null</tt> is given,
     *            default background color will be set.
     * @exception IllegalArgumentException
     *                if given color is disposed
     */
    public void setBackgroundInPlotArea(Color color) {
        if (color != null && color.isDisposed()) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }
        plotArea.setBackground(color);
    }

    /**
     * Sets the state of chart orientation. The horizontal orientation means
     * that X axis is horizontal as usual, while the vertical orientation means
     * that Y axis is horizontal.
     * 
     * @param orientation
     *            the orientation which can be SWT.HORIZONTAL or SWT.VERTICAL
     */
    public void setOrientation(int orientation) {
        if (orientation == SWT.HORIZONTAL || orientation == SWT.VERTICAL) {
            this.orientation = orientation;
        }
        updateLayout();
    }

    /**
     * Gets the state of chart orientation. The horizontal orientation means
     * that X axis is horizontal as usual, while the vertical orientation means
     * that Y axis is horizontal.
     * 
     * @return the orientation which can be SWT.HORIZONTAL or SWT.VERTICAL
     */
    public int getOrientation() {
        return orientation;
    }

    /**
     * Enables compressing series. By default, compressing series is enabled,
     * and normally there should be no usecase to disable it. However, if you
     * suspect that something is wrong in compressing series, you can disable it
     * to isolate the issue.
     * 
     * @param enabled
     *            true if enabling compressing series
     */
    public void enableCompress(boolean enabled) {
        compressEnabled = enabled;
    }

    /**
     * Gets the state indicating if compressing series is enabled.
     * 
     * @return true if compressing series is enabled
     */
    public boolean isCompressEnabled() {
        return compressEnabled;
    }

    /*
     * @see Listener#handleEvent(Event)
     */
    public void handleEvent(Event event) {
        switch (event.type) {
        case SWT.Resize:
            updateLayout();
            redraw();
            break;
        default:
            break;
        }
    }

    /**
     * Updates the layout of chart elements.
     */
    public void updateLayout() {
        if (legend != null) {
            legend.updateLayoutData();
        }

        if (title != null) {
            title.updateLayoutData();
        }

        if (axisSet != null) {
            axisSet.updateLayoutData();
        }

        layout();

        if (axisSet != null) {
            axisSet.refresh();
        }
    }

    /*
     * @see Control#update()
     */
    @Override
    public void update() {
        super.update();
        for (Control child : getChildren()) {
            child.update();
        }
    }

    /*
     * @see Control#redraw()
     */
    @Override
    public void redraw() {
        super.redraw();
        for (Control child : getChildren()) {
            child.redraw();
        }
    }
}
