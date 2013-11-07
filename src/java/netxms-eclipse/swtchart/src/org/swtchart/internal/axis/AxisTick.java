/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved.
 *
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.axis;

import java.text.Format;
import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Rectangle;
import org.swtchart.Chart;
import org.swtchart.IAxisTick;
import org.swtchart.IAxis.Position;

/**
 * An axis tick.
 */
public class AxisTick implements IAxisTick {

    /** the chart */
    private Chart chart;

    /** the axis */
    private Axis axis;

    /** the axis tick labels */
    private AxisTickLabels axisTickLabels;

    /** the axis tick marks */
    private AxisTickMarks axisTickMarks;

    /** true if tick is visible */
    private boolean isVisible;

    /** the tick mark step hint */
    private int tickMarkStepHint;

    /** the tick label angle in degree */
    private int tickLabelAngle;

    /** the default tick mark step hint */
    private static final int DEFAULT_TICK_MARK_STEP_HINT = 64;

    /**
     * Constructor.
     *
     * @param chart
     *            the chart
     * @param axis
     *            the axis
     */
    protected AxisTick(Chart chart, Axis axis) {
        this.chart = chart;
        this.axis = axis;

        axisTickLabels = new AxisTickLabels(chart, axis);
        axisTickMarks = new AxisTickMarks(chart, axis);
        isVisible = true;
        tickLabelAngle = 0;
        tickMarkStepHint = DEFAULT_TICK_MARK_STEP_HINT;
    }

    /**
     * Gets the axis tick marks.
     *
     * @return the axis tick marks
     */
    public AxisTickMarks getAxisTickMarks() {
        return axisTickMarks;
    }

    /**
     * Gets the axis tick labels.
     *
     * @return the axis tick labels
     */
    public AxisTickLabels getAxisTickLabels() {
        return axisTickLabels;
    }

    /*
     * @see IAxisTick#setForeground(Color)
     */
    public void setForeground(Color color) {
        if (color != null && color.isDisposed()) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }
        axisTickMarks.setForeground(color);
        axisTickLabels.setForeground(color);
    }

    /*
     * @see IAxisTick#getForeground()
     */
    public Color getForeground() {
        return axisTickMarks.getForeground();
    }

    /*
     * @see IAxisTick#setFont(Font)
     */
    public void setFont(Font font) {
        if (font != null && font.isDisposed()) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }
        axisTickLabels.setFont(font);
        chart.updateLayout();
    }

    /*
     * @see IAxisTick#getFont()
     */
    public Font getFont() {
        return axisTickLabels.getFont();
    }

    /*
     * @see IAxisTick#isVisible()
     */
    public boolean isVisible() {
        return isVisible;
    }

    /*
     * @see IAxisTick#setVisible(boolean)
     */
    public void setVisible(boolean isVisible) {
        this.isVisible = isVisible;
        chart.updateLayout();
    }

    /*
     * @see IAxisTick#getTickMarkStepHint()
     */
    public int getTickMarkStepHint() {
        return tickMarkStepHint;
    }

    /*
     * @see IAxisTick#setTickMarkStepHint(int)
     */
    public void setTickMarkStepHint(int tickMarkStepHint) {
        if (tickMarkStepHint < MIN_GRID_STEP_HINT) {
            this.tickMarkStepHint = DEFAULT_TICK_MARK_STEP_HINT;
        } else {
            this.tickMarkStepHint = tickMarkStepHint;
        }
        chart.updateLayout();
    }

    /*
     * @see IAxisTick#getTickLabelAngle()
     */
    public int getTickLabelAngle() {
        return tickLabelAngle;
    }

    /*
     * @see IAxisTick#setTickLabelAngle(int)
     */
    public void setTickLabelAngle(int angle) {
        if (angle < 0 || 90 < angle) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }

        if (tickLabelAngle != angle) {
            tickLabelAngle = angle;
            chart.updateLayout();
        }
    }

    /*
     * @see IAxisTick#setFormat(Format)
     */
    public void setFormat(Format format) {
        axisTickLabels.setFormat(format);
        chart.updateLayout();
    }

    /*
     * @see IAxisTick#getFormat()
     */
    public Format getFormat() {
        return axisTickLabels.getFormat();
    }

    /*
     * @see IAxisTick#getBounds()
     */
    public Rectangle getBounds() {
        Rectangle r1 = axisTickMarks.getBounds();
        Rectangle r2 = axisTickLabels.getBounds();
        Position position = axis.getPosition();

        if (position == Position.Primary && axis.isHorizontalAxis()) {
            return new Rectangle(r1.x, r1.y, r1.width, r1.height + r2.height);
        } else if (position == Position.Secondary && axis.isHorizontalAxis()) {
            return new Rectangle(r1.x, r2.y, r1.width, r1.height + r2.height);
        } else if (position == Position.Primary && !axis.isHorizontalAxis()) {
            return new Rectangle(r2.x, r1.y, r1.width + r2.width, r1.height);
        } else if (position == Position.Secondary && !axis.isHorizontalAxis()) {
            return new Rectangle(r1.x, r1.y, r1.width + r2.width, r1.height);
        } else {
            throw new IllegalStateException("unknown axis position");
        }
    }

    /*
     * @see IAxisTick#getTickLabelValues()
     */
    public double[] getTickLabelValues() {
        List<Double> list = axisTickLabels.getTickLabelValues();

        double[] values = new double[list.size()];
        for (int i = 0; i < values.length; i++) {
            values[i] = list.get(i);
        }

        return values;
    }

    /**
     * Updates the tick around per 64 pixel.
     *
     * @param length
     *            the axis length
     */
    protected void updateTick(int length) {
        if (length <= 0) {
            axisTickLabels.update(1);
        } else {
            axisTickLabels.update(length);
        }
    }

    /**
     * Updates the tick layout.
     */
    protected void updateLayoutData() {
        axisTickLabels.updateLayoutData();
        axisTickMarks.updateLayoutData();
    }

}
