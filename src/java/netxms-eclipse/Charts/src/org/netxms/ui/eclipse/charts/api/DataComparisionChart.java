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
 * Interface for data comparision chart
 *
 */
public interface DataComparisionChart extends DataChart
{
	public static final int BAR_CHART = 0;
	public static final int PIE_CHART = 1;
	public static final int RADAR_CHART = 2;
	public static final int TUBE_CHART = 3;

	/**
	 * Add parameter
	 * 
	 * @param name parameter's name
	 * @param value parameter's initial value
	 * @return parameter's index (0 .. MAX_CHART_ITEMS-1)
	 */
	public abstract int addParameter(String name, double value);
	
	/**
	 * Update value for parameter
	 * 
	 * @param index parameter's index (0 .. MAX_CHART_ITEMS-1)
	 * @param value parameter's value
	 * @param updateChart if tru, chart will be updated (repainted)
	 */
	public abstract void updateParameter(int index, double value, boolean updateChart);

	/**
	 * Set chart type
	 * 
	 * @param chartType new chart type
	 */
	public abstract void setChartType(int chartType);

	/**
	 * Get current chart type
	 * 
	 * @return curently selected chart type
	 */
	public abstract int getChartType();
	
	/**
	 * Set chart transposed (swapping of axes).
	 * 
	 * @param transposed true for transposed chart
	 */
	public abstract void setTransposed(boolean transposed);
	
	/**
	 * Get "transposed" flag
	 * 
	 * @return "transposed" flag
	 */
	public abstract boolean isTransposed();
}
