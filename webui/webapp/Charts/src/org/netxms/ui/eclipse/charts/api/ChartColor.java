/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.api;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.graphics.RGB;
import org.netxms.ui.eclipse.charts.Activator;

/**
 * Color for chart element
 *
 */
public final class ChartColor
{
	public int red;
	public int green;
	public int blue;
	public int alpha;
	
	/**
	 * Create color object from separate R, G, B, A values
	 * 
	 * @param red
	 * @param green
	 * @param blue
	 * @param alpha
	 */
	public ChartColor(int red, int green, int blue, int alpha)
	{
		this.red = red;
		this.green = green;
		this.blue = blue;
		this.alpha = alpha;
	}

	/**
	 * Create color object from separate R, G, B values. Alpha value set to 255 (opaque).
	 * 
	 * @param red
	 * @param green
	 * @param blue
	 */
	public ChartColor(int red, int green, int blue)
	{
		this.red = red;
		this.green = green;
		this.blue = blue;
		this.alpha = 255;
	}
	
	/**
	 * Create color object from 32bit integer containing RGB value (high-order byte ignored).
	 * 
	 * @param rgb RGB value encoded as 32bit integer
	 */
	public ChartColor(int rgb)
	{
		red = rgb & 0xFF;
		green = (rgb >> 8) & 0xFF;
		blue = rgb >> 16;
		alpha = 255;
	}
	
	/**
	 * Create chart color from RGB object
	 * 
	 * @param rgb RGB object
	 */
	public ChartColor(RGB rgb)
	{
		red = rgb.red;
		green = rgb.green;
		blue = rgb.blue;
		alpha = 255;
	}
	
	/**
	 * Create RGB value encoded into 32bit integer. Alpha value ignored.
	 *  
	 * @return RGB value for color
	 */
	public int getRGB()
	{
		return (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
	}
	
	/**
	 * Create RGBA value encoded into 32bit integer, with alpha value in high-order byte.
	 *  
	 * @return RGBA value for color
	 */
	public int getRGBA()
	{
		return (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16) | ((alpha & 0xFF) << 24);
	}
	
	/**
	 * Get color as SWT RGB object
	 * 
	 * @return RGB object
	 */
	public RGB getRGBObject()
	{
		return new RGB(red, green, blue);
	}
	
	/**
	 * Convenient method for creating chart color object from preference store
	 * 
	 * @param preferenceStore preference store
	 * @param name preference name
	 * @return chart color object
	 */
	public static ChartColor createFromPreferences(IPreferenceStore preferenceStore, String name)
	{
		RGB rgb = PreferenceConverter.getColor(preferenceStore, name);
		return new ChartColor(rgb.red, rgb.green, rgb.blue);
	}
	
	/**
	 * Get default color for series
	 * 
	 * @param index series index
	 * @return default color for series
	 */
	public static ChartColor getDefaultColor(int index)
	{
		return new ChartColor(PreferenceConverter.getColor(Activator.getDefault().getPreferenceStore(), "Chart.Colors.Data." + index)); //$NON-NLS-1$
	}
}
