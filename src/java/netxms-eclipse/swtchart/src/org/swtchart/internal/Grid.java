/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal;

import java.util.ArrayList;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.widgets.Display;
import org.swtchart.IGrid;
import org.swtchart.LineStyle;
import org.swtchart.internal.axis.Axis;

/**
 * A grid.
 */
public class Grid implements IGrid {

    /** the axis */
    private Axis axis;

    /** the grid color */
    private Color color;

    /** the visibility state */
    private boolean isVisible;

    /** the line style */
    private LineStyle lineStyle;

    /** the line width */
    private final static int LINE_WIDTH = 1;

    /** the default style */
    private final static LineStyle DEFAULT_STYLE = LineStyle.DOT;

    /** the default color */
    private final static int DEFAULT_FOREGROUND = SWT.COLOR_GRAY;

    /**
     * Constructor.
     * 
     * @param axis
     *            the axis
     */
    public Grid(Axis axis) {
        this.axis = axis;

        color = Display.getDefault().getSystemColor(DEFAULT_FOREGROUND);
        lineStyle = DEFAULT_STYLE;
        isVisible = true;
    }

    /*
     * @see IGrid#getForeground()
     */
    public Color getForeground() {
        if (color.isDisposed()) {
            color = Display.getDefault().getSystemColor(DEFAULT_FOREGROUND);
        }
        return color;
    }

    /*
     * @see IGrid#setForeground(Color)
     */
    public void setForeground(Color color) {
        if (color != null && color.isDisposed()) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }

        if (color == null) {
            this.color = Display.getDefault()
                    .getSystemColor(DEFAULT_FOREGROUND);
        } else {
            this.color = color;
        }
    }

    /*
     * @see IGrid#getStyle()
     */
    public LineStyle getStyle() {
        return lineStyle;
    }

    /*
     * @see IGrid#setStyle(LineStyle)
     */
    public void setStyle(LineStyle style) {
        if (style == null) {
            this.lineStyle = DEFAULT_STYLE;
        } else {
            this.lineStyle = style;
        }
    }

    /**
     * Draws grid.
     * 
     * @param gc
     *            the graphics context
     * @param width
     *            the width to draw grid
     * @param height
     *            the height to draw grid
     */
    protected void draw(GC gc, int width, int height) {
        if (!isVisible || lineStyle.equals(LineStyle.NONE)) {
            return;
        }

        int xWidth;
        if (axis.isHorizontalAxis()) {
            xWidth = width;
        } else {
            xWidth = height;
        }

        gc.setForeground(getForeground());
        ArrayList<Integer> tickLabelPosition = axis.getTick()
                .getAxisTickLabels().getTickLabelPositions();

        gc.setLineStyle(Util.getIndexDefinedInSWT(lineStyle));
        if (axis.isValidCategoryAxis()) {
            int step = 0;
            if (tickLabelPosition.size() > 1) {
                step = tickLabelPosition.get(1).intValue()
                        - tickLabelPosition.get(0).intValue();
            } else {
                step = xWidth;
            }
            int x = (int) (tickLabelPosition.get(0).intValue() - step / 2d);

            for (int i = 0; i < tickLabelPosition.size() + 1; i++) {
                x += step;
                if (x >= xWidth) {
                    continue;
                }

                if (axis.isHorizontalAxis()) {
                    gc.drawLine(x, LINE_WIDTH, x, height - LINE_WIDTH);
                } else {
                    gc.drawLine(LINE_WIDTH, x, width - LINE_WIDTH, x);
                }
            }
        } else {
            for (int i = 0; i < tickLabelPosition.size(); i++) {
                int x = tickLabelPosition.get(i).intValue();
                if (x >= xWidth) {
                    continue;
                }

                if (axis.isHorizontalAxis()) {
                    gc.drawLine(x, LINE_WIDTH, x, height - LINE_WIDTH);
                } else {
                    gc.drawLine(LINE_WIDTH, height - 1 - x, width - LINE_WIDTH,
                            height - 1 - x);
                }
            }
        }
    }
}
