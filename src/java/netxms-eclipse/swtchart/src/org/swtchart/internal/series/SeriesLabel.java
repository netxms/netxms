/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal.series;

import java.text.DecimalFormat;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Display;
import org.swtchart.ISeriesLabel;
import org.swtchart.internal.Util;

/**
 * A series label.
 */
public class SeriesLabel implements ISeriesLabel {

    /** the visibility state of series label */
    private boolean isVisible;

    /** the series label font */
    protected Font font;

    /** the series label color */
    protected Color color;

    /** the format for series label */
    private String format;

    /** the formats for series labels */
    private String[] formats;

    /** the default label color */
    private static final int DEFAULT_COLOR = SWT.COLOR_BLACK;

    /** the default font */
    private static final Font DEFAULT_FONT = Display.getDefault()
            .getSystemFont();

    /** the default label format */
    private static final String DEFAULT_FORMAT = "#.###########";

    /**
     * Constructor.
     */
    public SeriesLabel() {
        font = DEFAULT_FONT;
        color = Display.getDefault().getSystemColor(DEFAULT_COLOR);
        isVisible = false;
        format = DEFAULT_FORMAT;
    }

    /*
     * @see ISeriesLabel#getFormat()
     */
    public String getFormat() {
        return format;
    }

    /*
     * @see ISeriesLabel#setFormat(String)
     */
    public void setFormat(String format) {
        if (format == null) {
            this.format = DEFAULT_FORMAT;
        } else {
            this.format = format;
        }
    }

    /*
     * @see ISeriesLabel#getFormats()
     */
    public String[] getFormats() {
        if (formats == null) {
            return null;
        }

        String[] copiedFormats = new String[formats.length];
        System.arraycopy(formats, 0, copiedFormats, 0, formats.length);

        return copiedFormats;
    }

    /*
     * @see ISeriesLabel#setFormats(String[])
     */
    public void setFormats(String[] formats) {
        if (formats == null) {
            this.formats = null;
            return;
        }

        this.formats = new String[formats.length];
        System.arraycopy(formats, 0, this.formats, 0, formats.length);
    }

    /*
     * @see ISeriesLabel#getForeground()
     */
    public Color getForeground() {
        return color;
    }

    /*
     * @see ISeriesLabel#setForeground(Color)
     */
    public void setForeground(Color color) {
        if (color != null && color.isDisposed()) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }

        if (color == null) {
            this.color = Display.getDefault().getSystemColor(DEFAULT_COLOR);
        } else {
            this.color = color;
        }
    }

    /*
     * @see ISeriesLabel#getFont()
     */
    public Font getFont() {
        if (font.isDisposed()) {
            font = DEFAULT_FONT;
        }
        return font;
    }

    /*
     * @see ISeriesLabel#setFont(Font)
     */
    public void setFont(Font font) {
        if (font != null && font.isDisposed()) {
            SWT.error(SWT.ERROR_INVALID_ARGUMENT);
        }
        if (font == null) {
            this.font = DEFAULT_FONT;
        } else {
            this.font = font;
        }
    }

    /*
     * @see ISeriesLabel#isVisible()
     */
    public boolean isVisible() {
        return isVisible;
    }

    /*
     * @see ISeriesLabel#setVisible(boolean)
     */
    public void setVisible(boolean visible) {
        this.isVisible = visible;
    }

    /**
     * Draws series label.
     * 
     * @param gc
     *            the GC object
     * @param h
     *            the horizontal coordinate to draw label
     * @param v
     *            the vertical coordinate to draw label
     * @param ySeriesValue
     *            the Y series value
     * @param seriesIndex
     *            the series index
     * @param alignment
     *            the alignment of label position (SWT.CENTER or SWT.BOTTOM)
     */
    protected void draw(GC gc, int h, int v, double ySeriesValue,
            int seriesIndex, int alignment) {
        if (!isVisible) {
            return;
        }

        gc.setForeground(color);
        gc.setFont(getFont());

        // get format
        String format1 = format;
        if (formats != null && formats.length > seriesIndex) {
            format1 = formats[seriesIndex];
        }
        if (format1 == null || format1.equals("")) {
            return;
        }

        // get text
        String text;
        if (isDecimalFormat(format1)) {
            text = new DecimalFormat(format1).format(ySeriesValue);
        } else {
            text = format1.replaceAll("'", "");
        }

        // draw label
        if (alignment == SWT.CENTER) {
            Point p = Util.getExtentInGC(font, text);
            gc.drawString(text, (int) (h - p.x / 2d), (int) (v - p.y / 2d),
                    true);
        } else if (alignment == SWT.BOTTOM) {
            gc.drawString(text, h, v, true);
        }
    }

    /**
     * Gets the state indicating if decimal format is given.
     * 
     * @param text
     *            the text to be checked
     * @return true if decimal format is given
     */
    private boolean isDecimalFormat(String text) {
        StringBuilder nonEscapedPart = new StringBuilder();
        String[] elements = text.split("'");
        if (elements != null) {
            for (int i = 0; i < elements.length; i += 2) {
                nonEscapedPart.append(elements[i]);
            }
        }

        if (nonEscapedPart.indexOf("#") == -1
                && nonEscapedPart.indexOf("0") == -1) {
            return false;
        }
        return true;
    }
}
