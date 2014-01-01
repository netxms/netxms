/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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

import java.util.Date;
import java.util.List;

import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.GraphItemStyle;

/**
 * Historical data chart interface
 *
 */
public interface HistoricalDataChart extends DataChart
{
	/**
	 * Set time range for chart.
	 * 
	 * @param from
	 * @param to
	 */
	public abstract void setTimeRange(Date from, Date to);
	
	/**
	 * @return the zoomEnabled
	 */
	public abstract boolean isZoomEnabled();

	/**
	 * @param zoomEnabled the zoomEnabled to set
	 */
	public abstract void setZoomEnabled(boolean enableZoom);

	/**
	 * @return the itemStyles
	 */
	public abstract List<GraphItemStyle> getItemStyles();

	/**
	 * @param itemStyles the itemStyles to set
	 */
	public abstract void setItemStyles(List<GraphItemStyle> itemStyles);

	/**
	 * Add new parameter to chart.
	 * 
	 * @param item parameter
	 * @return assigned index
	 */
	public abstract int addParameter(GraphItem item);
	
	/**
	 * Update data for parameter
	 * 
	 * @param index parameter's index
	 * @param data data for parameter
	 * @param updateChart if true, chart will be updated (repainted)
	 */
	public abstract void updateParameter(int index, DciData data, boolean updateChart);
	
	/**
	 * Adjust X axis to fit all data
	 * 
	 * @param repaint if true, chart will be repainted after change
	 */
	public abstract void adjustXAxis(boolean repaint);

	/**
	 * Adjust Y axis to fit all data
	 * 
	 * @param repaint if true, chart will be repainted after change
	 */
	public abstract void adjustYAxis(boolean repaint);
	
	/**
	 * Zoom in
	 */
	public abstract void zoomIn();
	
	/**
	 * Zoom out
	 */
	public abstract void zoomOut();
	
	/**
	 * Set stacked mode
	 * 
	 * @param stacked
	 */
	public abstract void setStacked(boolean stacked);
	
	/**
	 * Get current settings for "stacked" mode
	 * 
	 * @return
	 */
	public abstract boolean isStacked();
	
	/**
	 * Set extended legend mode (when legend shows min, max, and average values)
	 * 
	 * @param extended
	 */
	public void setExtendedLegend(boolean extended);
	
	/**
	 * Get current settings for extended legend mode
	 * 
	 * @return
	 */
	public boolean isExtendedLegend();
}
