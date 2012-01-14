/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.shared;

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;

/**
 * Shared colors
 */
public class SharedColors
{
	public static Color BLACK;
	public static Color RED;
	public static Color WHITE;
	
	/**
	 * Initialize static members. Intended to be called once by library activator.
	 */
	public static void init()
	{
		Display display = Display.getDefault();
		
		BLACK = new Color(display, 0, 0, 0);
		RED = new Color(display, 255, 0, 0);
		WHITE = new Color(display, 255, 255, 255);
	}
}
