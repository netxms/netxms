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
package org.netxms.ui.eclipse.tools;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.PlatformUI;

/**
 * Utility class for converting between different color representation formats
 *
 */
public class ColorConverter
{
	public static final Color WHITE = new Color(Display.getDefault(), 255, 255, 255);
	public static final Color BLACK = new Color(Display.getDefault(), 0, 0, 0);
	
	/**
	 * Create integer value from Red/Green/Blue
	 * 
	 * @param r red
	 * @param g green
	 * @param b blue
	 * @return
	 */
	public static int rgbToInt(RGB rgb)
	{
		return rgb.red | (rgb.green << 8) | (rgb.blue << 16);
	}
	
	/**
	 * Create RGB object from integer value
	 * 
	 * @param color color as integer value
	 * @return RGB object
	 */
	public static RGB rgbFromInt(int color)
	{
		return new RGB(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
	}


	/**
	 * Create Color object from integer RGB representation
	 * @param rgb color's rgb representation
	 * @return color object
	 */
	public static Color colorFromInt(int rgb)
	{
		// All colors on server stored as BGR: red in less significant byte and blue in most significant byte
		return new Color(PlatformUI.getWorkbench().getDisplay(), rgb & 0xFF, (rgb >> 8) & 0xFF, rgb >> 16);
	}

	/**
	 * Create color object from preference string
	 *  
	 * @param store preference store
	 * @param name preference name
	 * @return Color object
	 */
	public static Color getColorFromPreferences(IPreferenceStore store, final String name)
	{
		return new Color(PlatformUI.getWorkbench().getDisplay(), PreferenceConverter.getColor(store, name));
	}

	/**
	 * Get internal integer representation of color from preference string
	 *  
	 * @param store preference store
	 * @param name preference name
	 * @return color in BGR format
	 */
	public static int getColorFromPreferencesAsInt(IPreferenceStore store, final String name)
	{
		return rgbToInt(PreferenceConverter.getColor(store, name));
	}
	
	private static float lerp(float start, float end, float amount)
	{
		float difference = end - start;
		float adjusted = difference * amount;
		return start + adjusted;
	}
	
	/**
	 * Adjust given color in the direction of another color by given amount.
	 * For example, to make color 50% lighter:
	 * adjustColor(color, new Color(color.getDevice(), 255, 255, 255), 0.5f);
	 * 
	 * @param color
	 * @param direction
	 * @param amount
	 * @return
	 */
	public static Color adjustColor(Color color, Color direction, float amount)
	{
		float sr = color.getRed(), sg = color.getGreen(), sb = color.getBlue();
		float dr = direction.getRed(), dg = direction.getGreen(), db = direction.getBlue();
		return new Color(color.getDevice(), (int)lerp(sr, dr, amount), (int)lerp(sg, dg, amount), (int)lerp(sb, db, amount));
	}
}
