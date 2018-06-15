/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets.internal;

import java.util.List;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for "Object Details" element
 */
public class ObjectDetailsConfig extends DashboardElementConfig
{
   @Element(required=false)
   private String query = "";
   
   @ElementList(required=false)
   private List<ObjectProperty> properties;
   
   @Element(required=false)
   private int refreshRate = 60; 
   
   /**
    * Create "object details" settings object from XML document
    * 
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static ObjectDetailsConfig createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(ObjectDetailsConfig.class, xml);
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
    * @return the properties
    */
   public List<ObjectProperty> getProperties()
   {
      return properties;
   }

   /**
    * @param properties the properties to set
    */
   public void setProperties(List<ObjectProperty> properties)
   {
      this.properties = properties;
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
      
      @Element
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
