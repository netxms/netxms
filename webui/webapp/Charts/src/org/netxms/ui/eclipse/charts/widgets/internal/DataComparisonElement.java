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
package org.netxms.ui.eclipse.charts.widgets.internal;

import org.eclipse.birt.chart.model.component.Series;
import org.netxms.client.datacollection.GraphItem;
import org.netxms.client.datacollection.Threshold;

/**
 * Single element used in data comparison charts
 *
 */
public class DataComparisonElement
{
	private GraphItem dci;
	private Threshold[] thresholds;
	private double value;
	private Series series; 
	
	/**
	 * @param name
	 * @param value
	 */
	public DataComparisonElement(GraphItem dci, double value)
	{
		this.dci = dci;
		this.value = value;
		series = null;
		thresholds = new Threshold[0];
	}
	
	/**
	 * @return the value
	 */
	public double getValue()
	{
		return value;
	}
	
	/**
	 * @param value the value to set
	 */
	public void setValue(double value)
	{
		this.value = value;
	}
	
	/**
	 * @return the name
	 */
	public String getName()
	{
		return dci.getDescription();
	}
	
	/**
	 * Get DCI object
	 * @return
	 */
	public GraphItem getObject()
	{
		return dci;
	}

	/**
	 * @return the series
	 */
	public Series getSeries()
	{
		return series;
	}

	/**
	 * @param series the series to set
	 */
	public void setSeries(Series series)
	{
		this.series = series;
	}

	/**
	 * @return the thresholds
	 */
	public Threshold[] getThresholds()
	{
		return thresholds;
	}

	/**
	 * @param thresholds the thresholds to set
	 */
	public void setThresholds(Threshold[] thresholds)
	{
		this.thresholds = thresholds;
	}
	
	/**
	 * Get max. threshold value for thresholds with operation >, >=, or ==
	 * @param defval default value to return when no suitable thresholds found
	 * @return
	 */
	public double getMaxThresholdValue(double defval)
	{
		double value = 0;
		for(Threshold t : thresholds)
		{
			if ((t.getOperation() == Threshold.OP_EQ) ||
			    (t.getOperation() == Threshold.OP_GT_EQ) ||
			    (t.getOperation() == Threshold.OP_GT))
			{
				try
				{
					double v = Double.parseDouble(t.getValue());
					if (v > value)
						value = v;
				}
				catch(NumberFormatException e)
				{
				}
			}
		}
		return (value > 0) ? value : defval;
	}

	/**
	 * Get minimal threshold value for thresholds with operation >, >=, or ==
	 * @param defval default value to return when no suitable thresholds found
	 * @return
	 */
	public double getMinThresholdValue(double defval)
	{
		double value = Double.MAX_VALUE;
		for(Threshold t : thresholds)
		{
			if ((t.getOperation() == Threshold.OP_EQ) ||
			    (t.getOperation() == Threshold.OP_GT_EQ) ||
			    (t.getOperation() == Threshold.OP_GT))
			{
				try
				{
					double v = Double.parseDouble(t.getValue());
					if (v < value)
						value = v;
				}
				catch(NumberFormatException e)
				{
				}
			}
		}
		return (value < Double.MAX_VALUE) ? value : defval;
	}
}
