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
 * Configuration for label
 */
@Root(name = "element", strict = false)
public class NetworkMapConfig extends DashboardElementConfig
{
	@Element(required=true)
	private long objectId = 0;

	@Element(required=false)
	private int zoomLevel = 100;	// in percents

   @Element(required=false)
	private boolean objectDoubleClickEnabled = false;

   @Element(required=false)
   private boolean hideLinkLabelsEnabled = false;

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
	 * @return the zoomLevel
	 */
	public int getZoomLevel()
	{
		return zoomLevel;
	}

	/**
	 * @param zoomLevel the zoomLevel to set
	 */
	public void setZoomLevel(int zoomLevel)
	{
		this.zoomLevel = zoomLevel;
	}

   /**
    * @return the objectDoubleClickEnabled
    */
   public boolean isObjectDoubleClickEnabled()
   {
      return objectDoubleClickEnabled;
   }

   /**
    * @param objectDoubleClickEnabled the objectDoubleClickEnabled to set
    */
   public void setObjectDoubleClickEnabled(boolean objectDoubleClickEnabled)
   {
      this.objectDoubleClickEnabled = objectDoubleClickEnabled;
   }

   /**
    * @return the hideLinkLabelsEnabled
    */
   public boolean isHideLinkLabelsEnabled()
   {
      return hideLinkLabelsEnabled;
   }

   /**
    * @param hideLinkLabelsEnabled the hideLinkLabelsEnabled to set
    */
   public void setHideLinkLabelsEnabled(boolean hideLinkLabelsEnabled)
   {
      this.hideLinkLabelsEnabled = hideLinkLabelsEnabled;
   }
}
