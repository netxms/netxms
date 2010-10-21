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

/**
 * Generic data chart interface
 *
 */
public interface DataChart
{
	public static final int MAX_CHART_ITEMS = 16;
	
	/**
	 * Marks end of initialization stage and causes first render of a chart.
	 */
	public abstract void initializationComplete();
	
	/**
	 * Set chart's title
	 * 
	 * @param title new title
	 */
	public abstract void setTitle(String title);
	
	/**
	 * Get chart's title
	 * 
	 * @return chart's title
	 */
	public abstract String getTitle();
	
	/**
	 * Show or hide chart's title
	 * 
	 * @param visible true to show title, false to hide
	 */
	public abstract void setTitleVisible(boolean visible);
	
	/**
	 * Get title's visibility
	 * 
	 * @return true if title is visible
	 */
	public abstract boolean isTitleVisible();

	/**
	 * Show or hide chart's legend
	 * 
	 * @param visible true to show legend, false to hide
	 */
	public abstract void setLegendVisible(boolean visible);
	
	/**
	 * Get legend's visibility
	 * 
	 * @return true if legend is visible
	 */
	public abstract boolean isLegendVisible();
	
	/**
	 * Set palette.
	 * 
	 * @param colors colors for series
	 */
	public abstract void setPalette(ChartColor[] colors);
	
	/**
	 * Set single palette element.
	 * 
	 * @param index element index
	 * @param color color for series
	 */
	public abstract void setPaletteEntry(int index, ChartColor color);

	/**
	 * Set 3D display mode
	 * 
	 * @param enabled true to enable 3D mode
	 */
	public abstract void set3DModeEnabled(boolean enabled);
	
	/**
	 * Get 3D mode state.
	 * 
	 * @return true if 3D display mode enabled
	 */
	public abstract boolean is3DModeEnabled();

	/**
	 * Refresh (repaint) chart using current data and settings
	 */
	public abstract void refresh();
}
