/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import java.util.Map;
import java.util.Set;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Configuration for availability chart widget
 */
@Root(name = "element", strict = false)
public class AvailabilityChartConfig extends DashboardElementConfig
{
   public static final int TODAY       = 0x00;
   public static final int YESTERDAY   = 0x01;
   public static final int THIS_WEEK   = 0x02;
   public static final int LAST_WEEK   = 0x03;
   public static final int THIS_MONTH  = 0x04;
   public static final int LAST_MONTH  = 0x05;
   public static final int THIS_YEAR   = 0x06;
   public static final int LAST_YEAR   = 0x07;
   public static final int CUSTOM      = 0x08;

   @Element(required = true)
	private long objectId = 0;

   @Element(required = false)
   private int period = TODAY;

   @Element(required = false)
   private int numberOfDays = 1; // Number of days for period type LAST_N_DAYS

	@Element(required = false)
   private int legendPosition = ChartConfiguration.POSITION_RIGHT;

	@Element(required = false)
	private boolean showLegend = true;

	@Element(required = false)
	private boolean translucent = false;

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#getObjects()
    */
	@Override
	public Set<Long> getObjects()
	{
		Set<Long> objects = super.getObjects();
		objects.add(objectId);
		return objects;
	}

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#remapObjects(java.util.Map)
    */
	@Override
	public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
	{
		super.remapObjects(remapData);
		ObjectIdMatchingData md = remapData.get(objectId);
		if (md != null)
			objectId = md.dstId;
	}

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setObjectId(long objectId)
	{
		this.objectId = objectId;
	}

	/**
    * @return the period
    */
   public int getPeriod()
   {
      return period;
   }

   /**
    * @param period the period to set
    */
   public void setPeriod(int period)
   {
      this.period = period;
   }

   /**
    * @return the numberOfDays
    */
   public int getNumberOfDays()
   {
      return numberOfDays;
   }

   /**
    * @param numberOfDays the numberOfDays to set
    */
   public void setNumberOfDays(int numberOfDays)
   {
      this.numberOfDays = numberOfDays;
   }

   /**
    * @return the legendPosition
    */
	public int getLegendPosition()
	{
		return legendPosition;
	}

	/**
	 * @param legendPosition the legendPosition to set
	 */
	public void setLegendPosition(int legendPosition)
	{
		this.legendPosition = legendPosition;
	}

	/**
	 * @return the showLegend
	 */
	public boolean isShowLegend()
	{
		return showLegend;
	}

	/**
	 * @param showLegend the showLegend to set
	 */
	public void setShowLegend(boolean showLegend)
	{
		this.showLegend = showLegend;
	}

	/**
	 * @return the translucent
	 */
	public boolean isTranslucent()
	{
		return translucent;
	}

	/**
	 * @param translucent the translucent to set
	 */
	public void setTranslucent(boolean translucent)
	{
		this.translucent = translucent;
	}
}
