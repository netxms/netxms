/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2026 Raden Solutions
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
 * Configuration for "AI observation monitor" dashboard element
 */
@Root(name = "element", strict = false)
public class AiObservationMonitorConfig extends DashboardElementConfig
{
   @Element(required = false)
   private long objectId = 0;

   @Element(required = false)
   private int instanceId = 0;

   @Element(required = false)
   private boolean newOnly = true;

   @Element(required = false)
   private int maxRecords = 50;

   @Element(required = false)
   private int refreshInterval = 60;

   /**
    * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#getObjects()
    */
   @Override
   public Set<Long> getObjects()
   {
      Set<Long> objects = super.getObjects();
      objects.add(objectId);
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
    * @return the instanceId
    */
   public int getInstanceId()
   {
      return instanceId;
   }

   /**
    * @param instanceId the instanceId to set
    */
   public void setInstanceId(int instanceId)
   {
      this.instanceId = instanceId;
   }

   /**
    * @return true if only observations in "new" state should be shown
    */
   public boolean isNewOnly()
   {
      return newOnly;
   }

   /**
    * @param newOnly true to show only observations in "new" state
    */
   public void setNewOnly(boolean newOnly)
   {
      this.newOnly = newOnly;
   }

   /**
    * @return the maxRecords
    */
   public int getMaxRecords()
   {
      return maxRecords;
   }

   /**
    * @param maxRecords the maxRecords to set
    */
   public void setMaxRecords(int maxRecords)
   {
      this.maxRecords = maxRecords;
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
}
