/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.charts.api;

import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.swt.graphics.RGB;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.resources.ThemeEngine;

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
      return new ChartColor(PreferenceStore.getInstance().getAsColor(name));
	}

	/**
	 * Get default color for series
	 *
	 * @param index series index
	 * @return default color for series
	 */
	public static ChartColor getDefaultColor(int index)
	{
      if (index < ChartConfiguration.DEFAULT_PALETTE_SIZE)
         return new ChartColor(ThemeEngine.getForegroundColorDefinition("Chart.Data." + (index + 1)));
      return generateColor(index - ChartConfiguration.DEFAULT_PALETTE_SIZE);
	}

   /**
    * Generate a visually distinct color for given index using golden angle hue distribution.
    *
    * @param index zero-based index into generated palette
    * @return generated color
    */
   private static ChartColor generateColor(int index)
   {
      double hue = (index * 137.508) % 360.0;
      double saturation = 0.55 + 0.15 * (index % 3);
      double brightness = 0.95 - 0.10 * ((index / 3) % 3);

      double c = brightness * saturation;
      double x = c * (1.0 - Math.abs((hue / 60.0) % 2.0 - 1.0));
      double m = brightness - c;

      double r, g, b;
      if (hue < 60)
      {
         r = c; g = x; b = 0;
      }
      else if (hue < 120)
      {
         r = x; g = c; b = 0;
      }
      else if (hue < 180)
      {
         r = 0; g = c; b = x;
      }
      else if (hue < 240)
      {
         r = 0; g = x; b = c;
      }
      else if (hue < 300)
      {
         r = x; g = 0; b = c;
      }
      else
      {
         r = c; g = 0; b = x;
      }

      return new ChartColor((int)((r + m) * 255), (int)((g + m) * 255), (int)((b + m) * 255));
   }
}
