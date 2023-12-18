/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Configuration for status map widget
 */
@Root(name = "element", strict = false)
public class StatusMapConfig extends DashboardElementConfig
{
	@Element(required=true)
	private long objectId = 0;

	@Element(required=false)
	private int severityFilter = 0xFF;
	
   @Element(required = false)
   private boolean hideObjectsInMaintenance = false;

	@Element(required=false)
	private boolean groupObjects = false;

   @Element(required=false)
   private boolean showTextFilter = false;

   @Element(required=false)
   private boolean showRadial = false;

   @Element(required=false)
   private boolean fitToScreen = false;

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
	 * @return the severityFilter
	 */
	public int getSeverityFilter()
	{
		return severityFilter;
	}

	/**
	 * @param severityFilter the severityFilter to set
	 */
	public void setSeverityFilter(int severityFilter)
	{
		this.severityFilter = severityFilter;
	}

	public boolean isGroupObjects()
	{
		return groupObjects;
	}

	public void setGroupObjects(boolean groupObjects)
	{
		this.groupObjects = groupObjects;
	}

   /**
    * @return the hideObjectsInMaintenance
    */
   public boolean isHideObjectsInMaintenance()
   {
      return hideObjectsInMaintenance;
   }

   /**
    * @param hideObjectsInMaintenance the hideObjectsInMaintenance to set
    */
   public void setHideObjectsInMaintenance(boolean hideObjectsInMaintenance)
   {
      this.hideObjectsInMaintenance = hideObjectsInMaintenance;
   }

   /**
    * @return the showTextFilter
    */
   public boolean isShowTextFilter()
   {
      return showTextFilter;
   }

   /**
    * @param showTextFilter the showTextFilter to set
    */
   public void setShowTextFilter(boolean showTextFilter)
   {
      this.showTextFilter = showTextFilter;
   }

   /**
    * @return the showRadial
    */
   public boolean isShowRadial()
   {
      return showRadial;
   }

   /**
    * @param showRadial the showRadial to set
    */
   public void setShowRadial(boolean showRadial)
   {
      this.showRadial = showRadial;
   }

   /**
    * @return the fitToScreen
    */
   public boolean isFitToScreen()
   {
      return fitToScreen;
   }

   /**
    * @param fitToScreen the fitToScreen to set
    */
   public void setFitToScreen(boolean fitToScreen)
   {
      this.fitToScreen = fitToScreen;
   }
}
