/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal;

import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Display;

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
}
