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

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;

/**
 * Configuration for "Object Details" element
 */
@Root(name = "element", strict = false)
public class ObjectDetailsConfig extends DashboardElementConfig
{
   @Element(required = false)
   private String query = "";

   @Element(required = false)
   private long rootObjectId = 0;

   @ElementList(required = false)
   private List<ObjectProperty> properties;

   @Element(required = false)
   private int refreshRate = 60;

   @ElementList(required = false)
   private List<String> orderingProperties = null;

   @Element(required = false)
   private int recordLimit = 0;

   /**
    * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#getObjects()
    */
   @Override
   public Set<Long> getObjects()
   {
      Set<Long> objects = super.getObjects();
      objects.add(rootObjectId);
      return objects;
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig#remapObjects(java.util.Map)
    */
   @Override
   public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
   {
      super.remapObjects(remapData);
      ObjectIdMatchingData md = remapData.get(rootObjectId);
      if (md != null)
         rootObjectId = md.dstId;
   }

   /**
    * @return the query
    */
   public String getQuery()
   {
      return query;
   }

   /**
    * @param query the query to set
    */
   public void setQuery(String query)
   {
      this.query = query;
   }

   /**
    * @return the rootObjectId
    */
   public long getRootObjectId()
   {
      return rootObjectId;
   }

   /**
    * @param rootObjectId the rootObjectId to set
    */
   public void setRootObjectId(long rootObjectId)
   {
      this.rootObjectId = rootObjectId;
   }

   /**
    * @return the properties
    */
   public List<ObjectProperty> getProperties()
   {
      return properties == null ? new ArrayList<ObjectProperty>() : properties;
   }

   /**
    * @param properties the properties to set
    */
   public void setProperties(List<ObjectProperty> properties)
   {
      this.properties = properties;
   }

   /**
    * @return the orderingProperties
    */
   public List<String> getOrderingProperties()
   {
      return orderingProperties == null ? new ArrayList<String>() : orderingProperties;
   }
   
   /**
    * Get ordering properties as comma separated list
    * 
    * @return ordering properties as comma separated list
    */
   public String getOrderingPropertiesAsText()
   {
      if ((orderingProperties == null) || orderingProperties.isEmpty())
         return "";
      StringBuilder sb = new StringBuilder(orderingProperties.get(0));
      for(int i = 1; i < orderingProperties.size(); i++)
      {
         sb.append(',');
         sb.append(orderingProperties.get(i));
      }
      return sb.toString();
   }

   /**
    * Set ordering properties
    * 
    * @param orderingProperties ordering properties
    */
   public void setOrderingProperties(List<String> orderingProperties)
   {
      this.orderingProperties = orderingProperties;
   }

   /**
    * Set ordering properties
    * 
    * @param orderingProperties ordering properties
    */
   public void setOrderingProperties(String orderingProperties)
   {
      if ((orderingProperties == null) || orderingProperties.isEmpty())
      {
         this.orderingProperties = null;
      }
      else
      {
         String[] parts = orderingProperties.split(",");
         this.orderingProperties = new ArrayList<String>(parts.length);
         for(String p : parts)
         {
            p = p.trim();
            if (!p.isEmpty())
               this.orderingProperties.add(p);
         }
      }
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
    * @return the recordLimit
    */
   public int getRecordLimit()
   {
      return recordLimit;
   }

   /**
    * @param recordLimit the recordLimit to set
    */
   public void setRecordLimit(int recordLimit)
   {
      this.recordLimit = recordLimit;
   }

   /**
    * Object property descriptor
    */
   @Root(name="property", strict=false)
   public static class ObjectProperty
   {
      public static final int STRING = 0;
      public static final int INTEGER = 1;
      public static final int FLOAT = 2;
      
      @Element
      public String name;
      
      @Element(required = false)
      public String displayName;
      
      @Attribute(required=false)
      public int type;
      
      /**
       * Default constructor 
       */
      public ObjectProperty()
      {
         name = "";
         displayName = "";
         type = STRING;
      }

      /**
       * Copy constructor 
       */
      public ObjectProperty(ObjectProperty src)
      {
         this.name = src.name;
         this.displayName = src.displayName;
         this.type = src.type;
      }
   }
}
