/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.objects.queries;

import java.util.Map;
import java.util.Set;
import org.netxms.client.objects.AbstractObject;

/**
 * Result of object query
 */
public class ObjectQueryResult
{
   private AbstractObject object;
   private Map<String, String> properties;

   /**
    * Create new object query result.
    *
    * @param object object
    * @param properties retrieved properties
    */
   public ObjectQueryResult(AbstractObject object, Map<String, String> properties)
   {
      this.object = object;
      this.properties = properties;
   }

   /**
    * Get object
    *
    * @return the object
    */
   public AbstractObject getObject()
   {
      return object;
   }

   /**
    * Get all properties
    *
    * @return properties
    */
   public Map<String, String> getProperties()
   {
      return properties;
   }

   /**
    * Get all property names
    *
    * @return property names
    */
   public Set<String> getPropertyNames()
   {
      return properties.keySet();
   }

   /**
    * Get value for specific property.
    *
    * @param name property name
    * @return property value or null
    */
   public String getPropertyValue(String name)
   {
      return properties.get(name);
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ObjectQueryResult [object=" + object + ", properties=" + properties + "]";
   }
}
