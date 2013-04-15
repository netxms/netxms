/*******************************************************************************
 * Copyright (c) 2008-2011 SWTChart project. All rights reserved. 
 * 
 * This code is distributed under the terms of the Eclipse Public License v1.0
 * which is available at http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.swtchart.internal;

import org.eclipse.rap.rwt.graphics.Graphics;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Point;

/**
 * A utility class providing generic methods.
 */
@SuppressWarnings("deprecation")
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
		
		return Graphics.textExtent(font, text, 0);
	}
}
