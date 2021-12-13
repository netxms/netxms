/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Display;
import org.swtchart.LineStyle;

/**
 * A utility class providing generic methods.
 */
public final class Util
{
	/**
	 * Gets the text extent with given font in GC. If the given text or font is
	 * <code>null</code> or already disposed, point containing size zero will be
	 * returned.
	 * 
	 * @param font
	 *           the font
	 * @param text
	 *           the text
	 * @return a point containing text extent
	 */
	public static Point getExtentInGC(Font font, String text)
	{
		if (text == null || font == null || font.isDisposed())
		{
			return new Point(0, 0);
		}
		
		GC gc = new GC(Display.getCurrent());
		gc.setFont(font);
		Point e = gc.textExtent(text);
		gc.dispose();
		return e;
	}

    /**
     * Gets the index defined in SWT.
     * 
     * @param lineStyle
     *            the line style
     * @return the index defined in SWT.
     */
    public static int getIndexDefinedInSWT(LineStyle lineStyle) {
        switch (lineStyle) {
        case NONE:
            return SWT.NONE;
        case SOLID:
            return SWT.LINE_SOLID;
        case DASH:
            return SWT.LINE_DASH;
        case DOT:
            return SWT.LINE_DOT;
        case DASHDOT:
            return SWT.LINE_DASHDOT;
        case DASHDOTDOT:
            return SWT.LINE_DASHDOTDOT;
        default:
            return SWT.LINE_SOLID;
        }
    }
}
