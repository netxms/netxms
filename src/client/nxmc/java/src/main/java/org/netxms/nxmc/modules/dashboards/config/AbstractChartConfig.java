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
package org.netxms.nxmc.modules.dashboards.config;

import java.util.Map;
import java.util.Set;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.DciIdMatchingData;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;

/**
 * Base class for all chart widget configs
 */
@Root(name = "element", strict = false)
public abstract class AbstractChartConfig extends DashboardElementConfig
{
   @ElementArray(required = false)
	private ChartDciConfig[] dciList = new ChartDciConfig[0];

	@Element(required = false)
	private int legendPosition = ChartConfiguration.POSITION_BOTTOM;

	@Element(required = false)
	private boolean showLegend = true;

   @Element(required = false)
   private boolean extendedLegend = false;

   @Element(required = false)
   private boolean autoScale = true;

   @Element(required = false)
   private boolean translucent = false;

	@Element(required = false)
	private int refreshRate = 30;

   @Element(required = false)
   private double minYScaleValue = 0;

   @Element(required = false)
   private double maxYScaleValue = 100;

   @Element(required = false)
   private boolean modifyYBase = false;

   @Element(required = false)
   private String yAxisLabel = null;

   @Element(required=false)
   private long drillDownObjectId = 0;

	/**
	 * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#getObjects()
	 */
	@Override
	public Set<Long> getObjects()
	{
		Set<Long> objects = super.getObjects();
      objects.add(drillDownObjectId);
		for(ChartDciConfig dci : dciList)
			objects.add(dci.nodeId);
		return objects;
	}

	/**
	 * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#getDataCollectionItems()
	 */
	@Override
	public Map<Long, Long> getDataCollectionItems()
	{
		Map<Long, Long> items =  super.getDataCollectionItems();
		for(ChartDciConfig dci : dciList)
			items.put(dci.dciId, dci.nodeId);
		return items;
	}

   /**
    * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#remapObjects(java.util.Map)
    */
   @Override
   public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
   {
      super.remapObjects(remapData);
      ObjectIdMatchingData md = remapData.get(drillDownObjectId);
      if (md != null)
         drillDownObjectId = md.dstId;
   }

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#remapDataCollectionItems(java.util.Map)
    */
	@Override
	public void remapDataCollectionItems(Map<Long, DciIdMatchingData> remapData)
	{
		super.remapDataCollectionItems(remapData);
		for(ChartDciConfig dci : dciList)
		{
			DciIdMatchingData md = remapData.get(dci.dciId);
			if (md != null)
			{
				dci.dciId = md.dstDciId;
				dci.nodeId = md.dstNodeId;
			}
		}
	}

	/**
	 * @return the dciList
	 */
	public ChartDciConfig[] getDciList()
	{
		return dciList;
	}

	/**
	 * @param dciList the dciList to set
	 */
	public void setDciList(ChartDciConfig[] dciList)
	{
		this.dciList = dciList;
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
    * @return the extendedLegend
    */
   public boolean isExtendedLegend()
   {
      return extendedLegend;
   }

   /**
    * @param extendedLegend the extendedLegend to set
    */
   public void setExtendedLegend(boolean extendedLegend)
   {
      this.extendedLegend = extendedLegend;
   }

	/**
	 * @return the refreshRate
	 */
	public int getRefreshRate()
	{
		return refreshRate;
	}

	/**
	 * @param refreshRate the refreshRate to set
	 */
	public void setRefreshRate(int refreshRate)
	{
		this.refreshRate = refreshRate;
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

   /**
    * @return the minYScaleValue
    */
   public double getMinYScaleValue()
   {
      return minYScaleValue;
   }

   /**
    * @param minYScaleValue the minYScaleValue to set
    */
   public void setMinYScaleValue(double minYScaleValue)
   {
      this.minYScaleValue = minYScaleValue;
   }

   /**
    * @return the maxYScaleValue
    */
   public double getMaxYScaleValue()
   {
      return maxYScaleValue;
   }

   /**
    * @param maxYScaleValue the maxYScaleValue to set
    */
   public void setMaxYScaleValue(double maxYScaleValue)
   {
      this.maxYScaleValue = maxYScaleValue;
   }
   
   /**
    * @return
    */
   public boolean isAutoScale()
   {
      return autoScale;
   }

   /**
    * @param autoScale
    */
   public void setAutoScale(boolean autoScale)
   {
      this.autoScale = autoScale;
   }

   /**
    * Set modify Y base
    * 
    * @param modifyYBase if true, use min DCI value as Y base
    */
   public void setModifyYBase(boolean modifyYBase)
   {
      this.modifyYBase = modifyYBase;
   }
   
   /**
    * Modify Y base
    * 
    * @return true if use min DCI value as Y base
    */
   public boolean modifyYBase()
   {
      return modifyYBase;
   }

   /**
    * @return the yAxisLabel
    */
   public String getYAxisLabel()
   {
      return (yAxisLabel != null) ? yAxisLabel : "";
   }

   /**
    * @param yAxisLabel the yAxisLabel to set
    */
   public void setYAxisLabel(String yAxisLabel)
   {
      this.yAxisLabel = yAxisLabel;
   }

   /**
    * @return the drillDownObjectId
    */
   public long getDrillDownObjectId()
   {
      return drillDownObjectId;
   }

   /**
    * @param drillDownObjectId the drillDownObjectId to set
    */
   public void setDrillDownObjectId(long drillDownObjectId)
   {
      this.drillDownObjectId = drillDownObjectId;
   }
}
