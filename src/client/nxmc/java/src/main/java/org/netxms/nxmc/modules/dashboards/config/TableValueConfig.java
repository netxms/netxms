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

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.DciIdMatchingData;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;

/**
 * Configuration for table last value widget
 */
public class TableValueConfig extends DashboardElementConfig
{
	@Element(required=true)
	private long objectId = 0;

	@Element(required=true)
	private long dciId = 0;

   @Element(required = false)
   public String dciName;

   @Element(required = false)
   public String dciDescription;

	@Element(required = false)
	private int refreshRate = 30;

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
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#getDataCollectionItems()
    */
   @Override
   public Map<Long, Long> getDataCollectionItems()
   {
      Map<Long, Long> dcis = new HashMap<Long, Long>();
      dcis.put(dciId,  objectId);
      return dcis;
   }

   /**
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#remapDataCollectionItems(java.util.Map)
    */
   @Override
   public void remapDataCollectionItems(Map<Long, DciIdMatchingData> remapData)
   {
      super.remapDataCollectionItems(remapData);
      DciIdMatchingData md = remapData.get(dciId);
      if (md != null)
      {
         objectId = md.dstNodeId;
         dciId = md.dstDciId;
      }
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
	 * @return
	 */
	public long getDciId()
	{
		return dciId;
	}

	/**
	 * @param dciId
	 */
	public void setDciId(long dciId)
	{
		this.dciId = dciId;
	}

	/**
    * @return the dciName
    */
   public String getDciName()
   {
      return (dciName != null) ? dciName : "";
   }

   /**
    * @param dciName the dciName to set
    */
   public void setDciName(String dciName)
   {
      this.dciName = dciName;
   }

   /**
    * @return the dciDescription
    */
   public String getDciDescription()
   {
      return (dciDescription != null) ? dciDescription : "";
   }

   /**
    * @param dciDescription the dciDescription to set
    */
   public void setDciDescription(String dciDescription)
   {
      this.dciDescription = dciDescription;
   }

   /**
    * @return
    */
	public int getRefreshRate()
	{
		return refreshRate;
	}

	/**
	 * @param refreshRate
	 */
	public void setRefreshRate(int refreshRate)
	{
		this.refreshRate = refreshRate;
	}
}
