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
package org.netxms.ui.eclipse.tools;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;

/**
 * Simple color cache which takes care of color destruction when associated control disposed
 */
public class ColorCache implements DisposeListener
{
	private Map<RGB, Color> cache = new HashMap<RGB, Color>();

	/**
	 * Create unassociated color cache. It must be disposed explicitly by calling dispose().
	 */
	public ColorCache()
	{
	}
	
	/**
	 * Create color cache associated with given control. All colors in a cache will be disposed when
	 * associated control gets disposed.
	 * 
	 * @param control control
	 */
	public ColorCache(Control control)
	{
		control.addDisposeListener(this);
	}
	
	/**
	 * Create color from RGB object. If already presented in cache, return cached color.
	 * 
	 * @param rgb rgb
	 * @return color object
	 */
	public Color create(RGB rgb)
	{
		Color color = cache.get(rgb);
		if (color == null)
		{
			color = new Color(Display.getCurrent(), rgb);
			cache.put(rgb, color);
		}
		return color;
	}
	
	/**
	 * Create color with given red/green/blue values. If already presented in cache, return cached color.
	 * 
	 * @param r
	 * @param g
	 * @param b
	 * @return color object
	 */
	public Color create(int r, int g, int b)
	{
		return create(new RGB(r, g, b));
	}
	
	/**
	 * Dispose all colors in a cache
	 */
	public void dispose()
	{
		for(Color color : cache.values())
			color.dispose();
		cache.clear();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
	 */
	@Override
	public void widgetDisposed(DisposeEvent e)
	{
		dispose();
	}
}
