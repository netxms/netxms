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

import java.util.Date;

import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphItem;

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
}
