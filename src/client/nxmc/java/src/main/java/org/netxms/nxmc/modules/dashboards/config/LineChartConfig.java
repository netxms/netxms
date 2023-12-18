/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.config;

import org.netxms.client.constants.TimeUnit;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Configuration for line chart
 */
@Root(name = "element", strict = false)
public class LineChartConfig extends AbstractChartConfig
{
	@Element(required=false)
   private int timeUnits = TimeUnit.HOUR.getValue();

	@Element(required=false)
	private int timeRange = 1;

	@Element(required = false)
	private boolean showGrid = true;

   @Element(required = false)
   private boolean logScaleEnabled = false;

   @Element(required = false)
   private boolean useMultipliers = true;

   @Element(required = false)
   private boolean stacked = false;

   @Element(required = false)
   private int lineWidth = 2;

   @Element(required = false)
   private boolean area = false;

   @Element(required = false)
   private boolean interactive = false;

   /**
    * Default constructor
    */
   public LineChartConfig()
   {
      // for compatibility with older versions
      setTranslucent(true);
   }

	/**
	 * @return the timeUnits
	 */
	public int getTimeUnits()
	{
		return timeUnits;
	}

	/**
	 * @param timeUnits the timeUnits to set
	 */
	public void setTimeUnits(int timeUnits)
	{
		this.timeUnits = timeUnits;
	}

	/**
	 * @return the timeRange
	 */
	public int getTimeRange()
	{
		return timeRange;
	}

	/**
	 * @param timeRange the timeRange to set
	 */
	public void setTimeRange(int timeRange)
	{
		this.timeRange = timeRange;
	}

	/**
	 * Get time range covered by graph in milliseconds
	 * 
	 * @return
	 */
	public long getTimeRangeMillis()
	{
      switch(TimeUnit.getByValue(timeUnits))
		{
         case MINUTE:
				return (long)timeRange * 60L * 1000L;
         case HOUR:
				return (long)timeRange * 60L * 60L * 1000L;
         case DAY:
				return (long)timeRange * 24L * 60L * 60L * 1000L;
		}
		return 0;
	}

	/**
	 * @return the showGrid
	 */
	public boolean isShowGrid()
	{
		return showGrid;
	}

	/**
	 * @param showGrid the showGrid to set
	 */
	public void setShowGrid(boolean showGrid)
	{
		this.showGrid = showGrid;
	}

   /**
    * @return the logScaleEnabled
    */
   public boolean isLogScaleEnabled()
   {
      return logScaleEnabled;
   }

   /**
    * @param logScaleEnabled the logScaleEnabled to set
    */
   public void setLogScaleEnabled(boolean logScaleEnabled)
   {
      this.logScaleEnabled = logScaleEnabled;
   }

   /**
    * @return the useMultipliers
    */
   public boolean isUseMultipliers()
   {
      return useMultipliers;
   }

   /**
    * @param useMultipliers the useMultipliers to set
    */
   public void setUseMultipliers(boolean useMultipliers)
   {
      this.useMultipliers = useMultipliers;
   }

   /**
    * @return the stacked
    */
   public boolean isStacked()
   {
      return stacked;
   }

   /**
    * @param stacked the stacked to set
    */
   public void setStacked(boolean stacked)
   {
      this.stacked = stacked;
   }

   /**
    * @return the lineWidth
    */
   public int getLineWidth()
   {
      return lineWidth;
   }

   /**
    * @param lineWidth the lineWidth to set
    */
   public void setLineWidth(int lineWidth)
   {
      this.lineWidth = lineWidth;
   }

   /**
    * @return the area
    */
   public boolean isArea()
   {
      return area;
   }

   /**
    * @param area the area to set
    */
   public void setArea(boolean area)
   {
      this.area = area;
   }

   /**
    * @return the interactive
    */
   public boolean isInteractive()
   {
      return interactive;
   }

   /**
    * @param interactive the interactive to set
    */
   public void setInteractive(boolean interactive)
   {
      this.interactive = interactive;
   }
}
