/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;

/**
 * Configuration for alarm viewer widget
 */
@Root(name = "element", strict = false)
public class DciSummaryTableConfig extends DashboardElementConfig
{
	@Element(required=true)
	private long baseObjectId = 0;

   @Element(required=true)
   private int tableId = 0;

   @Element(required=false)
   private int refreshInterval = 0;
   
   @Element(required=false)
   private int numRowShown = 0;
   
   @ElementList(required = false, inline=true)
   private List<String> sortingColumnList = new ArrayList<String>();

   /**
    * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#getObjects()
    */
	@Override
	public Set<Long> getObjects()
	{
		Set<Long> objects = super.getObjects();
		objects.add(baseObjectId);
		return objects;
	}

   /**
    * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#remapObjects(java.util.Map)
    */
	@Override
	public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
	{
		super.remapObjects(remapData);
		ObjectIdMatchingData md = remapData.get(baseObjectId);
		if (md != null)
			baseObjectId = md.dstId;
	}

	/**
	 * @return the objectId
	 */
	public long getBaseObjectId()
	{
		return baseObjectId;
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setBaseObjectId(long objectId)
	{
		this.baseObjectId = objectId;
	}

   /**
    * @return the tableId
    */
   public int getTableId()
   {
      return tableId;
   }

   /**
    * @param tableId the tableId to set
    */
   public void setTableId(int tableId)
   {
      this.tableId = tableId;
   }

   /**
    * @return the refreshInterval
    */
   public int getRefreshInterval()
   {
      return refreshInterval;
   }

   /**
    * @param refreshInterval the refreshInterval to set
    */
   public void setRefreshInterval(int refreshInterval)
   {
      this.refreshInterval = refreshInterval;
   }

   /**
    * @return the numCollShown
    */
   public int getNumRowShown()
   {
      return numRowShown;
   }

   /**
    * @param numCollShown the numCollShown to set
    */
   public void setNumRowShown(int numRowShown)
   {
      this.numRowShown = numRowShown;
   }

   /**
    * @return the sortingColumnList
    */
   public List<String> getSortingColumnList()
   {
      return sortingColumnList;
   }

   /**
    * @param sortingColumnList the sortingColumnList to set
    */
   public void setSortingColumnList(List<String> sortingColumnList)
   {
      this.sortingColumnList = sortingColumnList;
   }   
}
