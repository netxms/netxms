/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved.
 *
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyleRange;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.TextLayout;
import org.eclipse.swt.graphics.Transform;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Display;
import org.swtchart.Chart;
import org.swtchart.Constants;
import org.swtchart.ITitle;

/**
 * A base class for title.
 */
public class Title extends Canvas implements ITitle, PaintListener {

    /** the chart */
    protected Chart chart;

    /** the title text */
    protected String text;

    /** the style ranges */
    private StyleRange[] styleRanges;

    /** The text layout */
    private final TextLayout textLayout;

    /** the visibility state of axis */
    protected boolean isVisible;

    /** the default font */
    private final Font defaultFont;

    /** the default font size */
    private static final int DEFAULT_FONT_SIZE = Constants.LARGE_FONT_SIZE;

    /** the default color */
    private static final int DEFAULT_FOREGROUND = SWT.COLOR_BLUE;

    /** the default text */
    private static final String DEFAULT_TEXT = "";

    /**
     * Constructor.
     * 
     * @param parent
     *            the parent composite
     * @param style
     *            the style
     */
    public Title(Chart parent, int style) {
        super(parent, style);

        this.chart = parent;
        text = DEFAULT_TEXT;
        isVisible = true;

        defaultFont = new Font(Display.getDefault(), "Tahoma", DEFAULT_FONT_SIZE, SWT.BOLD);
        textLayout = new TextLayout(Display.getDefault());

        setFont(defaultFont);
        setForeground(Display.getDefault().getSystemColor(DEFAULT_FOREGROUND));

        addPaintListener(this);
        addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				defaultFont.dispose();
				textLayout.dispose();
			}
		});
    }

    /*
     * @see ITitle#setText(String)
     */
    public void setText(String text) {
        String title;
        if (text == null) {
            title = getDefaultText();
        } else {
            title = text;
        }

        textLayout.setText(title);
        this.text = title;

        chart.updateLayout(); // text could be changed to blank
    }

    /**
     * Gets the default title text.
     * 
     * @return the default title text
     */
    protected String getDefaultText() {
        return DEFAULT_TEXT;
    }

    /*
     * @see ITitle#getText()
     */
    public String getText() {
        return text;
    }

    /*
     * @see Canvas#setFont(Font)
     */
    @Override
    public void setFont(Font font) {
        if (font == null) {
            super.setFont(defaultFont);
        } else {
            super.setFont(font);
        }
        chart.updateLayout();
    }

    /*
     * @see Control#setForeground(Color)
     */
    @Override
    public void setForeground(Color color) {
        if (color == null) {
            super.setForeground(Display.getDefault().getSystemColor(DEFAULT_FOREGROUND));
        } else {
            super.setForeground(color);
        }
    }

    /*
     * @see ITitle#setStyleRanges(StyleRange[])
     */
    public void setStyleRanges(StyleRange[] ranges) {
        styleRanges = ranges;
        if (styleRanges != null) {
            for (StyleRange range : styleRanges) {
                if (range != null) {
                    textLayout.setStyle(range, range.start, range.start
                            + range.length);
                }
            }
        }
        chart.updateLayout();
    }

    /*
     * @see ITitle#getStyleRanges()
     */
    public StyleRange[] getStyleRanges() {
        return styleRanges;
    }

    /*
     * @see Control#setVisible(boolean)
     */
    @Override
    public void setVisible(boolean isVisible) {
        if (this.isVisible == isVisible) {
            return;
        }

        this.isVisible = isVisible;
        chart.updateLayout();
    }

    /*
     * @see Control#isVisible()
     */
    @Override
    public boolean isVisible() {
        return isVisible;
    }

    /**
     * Gets the state indicating if showing title horizontally.
     * 
     * @return the state indicating if showing title horizontally
     */
    protected boolean isHorizontal() {
        return true;
    }

    /**
     * Updates the title layout data.
     */
    public void updateLayoutData() {
        int height;
        int width;
        if (isVisible() && !text.trim().equals("")) {
            if (styleRanges == null) {
                Point p = Util.getExtentInGC(getFont(), text);
                width = p.x;
                height = p.y;
            } else {
                Rectangle r = textLayout.getBounds();
                width = r.width;
                height = r.height;
            }
        } else {
            width = 0;
            height = 0;
        }

        if (isHorizontal()) {
            setLayoutData(new ChartLayoutData(width, height));
        } else {
            setLayoutData(new ChartLayoutData(height, width));
        }
    }

    /*
     * @see PaintListener#paintControl(PaintEvent)
     */
    public void paintControl(PaintEvent e) {
        if (text == null || text.equals("") || !isVisible) {
            return;
        }

        if (isHorizontal()) {
            drawHorizontalTitle(e.gc);
        } else {
            drawVerticalTitle(e.gc);
        }
    }

    /**
     * Draws the horizontal title.
     * 
     * @param gc
     *            The graphics context
     */
    private void drawHorizontalTitle(GC gc) {
        boolean useStyleRanges = styleRanges != null;

        int width = getSize().x;
        int textWidth;
        if (useStyleRanges) {
            textWidth = textLayout.getBounds().width;
        } else {
            textWidth = gc.textExtent(text).x;
        }

        int x = (int) (width / 2d - textWidth / 2d);
        if (x < 0) {
            // this happens when window size is too small
            x = 0;
        }

        if (useStyleRanges) {
            textLayout.draw(gc, x, 0);
        } else {
            gc.drawText(text, x, 0, true);
        }
    }

    /**
     * Draws the vertical title.
     * 
     * @param gc
     *            The graphics context
     */
    private void drawVerticalTitle(GC gc) {
        boolean useStyleRanges = styleRanges != null;

        int textWidth;
        int textHeight;
        if (useStyleRanges) {
            textWidth = textLayout.getBounds().width;
            textHeight = textLayout.getBounds().height;
        } else {
            textWidth = gc.textExtent(text).x;
            textHeight = gc.textExtent(text).y;
        }

        // create image to draw text
        Image image = new Image(Display.getCurrent(), textWidth, textHeight);
        GC tmpGc = new GC(image);

        if (useStyleRanges) {
            textLayout.draw(tmpGc, 0, 0);
        } else {
            tmpGc.setForeground(getForeground());
            tmpGc.setBackground(getBackground());
            tmpGc.setFont(getFont());
            tmpGc.drawText(text, 0, 0);
        }

        // set transform to rotate
        Transform transform = new Transform(gc.getDevice());
        transform.translate(0, textWidth);
        transform.rotate(270);
        gc.setTransform(transform);

        // draw the image on the rotated graphics context
        int height = getSize().y;
        int y = (int) (height / 2d - textWidth / 2d);
        if (y < 0) {
            y = 0;
        }
        gc.drawImage(image, -y, 0);

        // dispose resources
        tmpGc.dispose();
        transform.dispose();
        image.dispose();
    }
}