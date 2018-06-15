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
package org.netxms.client;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.netxms.client.objects.AbstractObject;

/**
 * Result of object query
 */
public class ObjectQueryResult
{
   private AbstractObject object;
   private Map<String, String> properties;
   
   /**
    * Create new result
    */
   protected ObjectQueryResult(AbstractObject object, List<String> properties, List<String> values)
   {
      this.object = object;
      this.properties = new HashMap<String, String>();
      for(int i = 0; i < properties.size(); i++)
      {
         this.properties.put(properties.get(i), values.get(i));
      }
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
    * Get value for specific property.
    * 
    * @param name property name
    * @return property value or null
    */
   public String getPropertyValue(String name)
   {
      return properties.get(name);
   }
}
