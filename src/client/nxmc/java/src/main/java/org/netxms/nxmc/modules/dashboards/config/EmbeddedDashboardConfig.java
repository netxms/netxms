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
import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import com.google.gson.Gson;

/**
 * Configuration for embedded dashboard element
 */
@Root(name = "element", strict = false)
public class EmbeddedDashboardConfig extends DashboardElementConfig
{
   @Element(required = false)
   private long objectId = 0;

   @Element(required = false)
   private long[] dashboardObjects = new long[0];

   @Element(required = false)
   private long contextObjectId = 0;

   @Element(required = false)
   private int displayInterval = 60;

	/**
	 * Create line chart settings object from XML document
	 * 
	 * @param data XML or JSON document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static EmbeddedDashboardConfig createFromXmlOrJson(final String data) throws Exception
	{

	   EmbeddedDashboardConfig config;
      if (data.trim().startsWith("<"))
      {
         config = XMLTools.createFromXml(EmbeddedDashboardConfig.class, data);
      }
      else
      {
         config = new Gson().fromJson(data, EmbeddedDashboardConfig.class);
      }

		// fix configuration if it was saved in old format
		if ((config.objectId != 0) && (config.dashboardObjects.length == 0))
		{
			config.dashboardObjects = new long[1];
			config.dashboardObjects[0] = config.objectId;
			config.objectId = 0;
		}
		return config;
	}

   /**
    * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#getObjects()
    */
	@Override
	public Set<Long> getObjects()
	{
		Set<Long> objects = super.getObjects();
		objects.add(objectId);
      objects.add(contextObjectId);
		for(long id : dashboardObjects)
			objects.add(id);
		return objects;
	}

   /**
    * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#remapObjects(java.util.Map)
    */
	@Override
	public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
	{
		super.remapObjects(remapData);
		ObjectIdMatchingData md = remapData.get(objectId);
		if (md != null)
			objectId = md.dstId;
		md = remapData.get(contextObjectId);
      if (md != null)
         contextObjectId = md.dstId;
		for(int i = 0; i < dashboardObjects.length; i++)
		{
			md = remapData.get(dashboardObjects[i]);
			if (md != null)
				dashboardObjects[i] = md.dstId;
		}
	}

	/**
	 * @return the dashboardObjects
	 */
	public long[] getDashboardObjects()
	{
		return dashboardObjects;
	}

	/**
	 * @param dashboardObjects the dashboardObjects to set
	 */
	public void setDashboardObjects(long[] dashboardObjects)
	{
		this.dashboardObjects = dashboardObjects;
	}

	/**
    * @return the contextObjectId
    */
   public long getContextObjectId()
   {
      return contextObjectId;
   }

   /**
    * @param contextObjectId the contextObjectId to set
    */
   public void setContextObjectId(long contextObjectId)
   {
      this.contextObjectId = contextObjectId;
   }

   /**
    * @return the displayInterval
    */
	public int getDisplayInterval()
	{
		return displayInterval;
	}

	/**
	 * @param displayInterval the displayInterval to set
	 */
	public void setDisplayInterval(int displayInterval)
	{
		this.displayInterval = displayInterval;
	}
}
