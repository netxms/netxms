/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.websvc.json;

import java.util.Set;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

/**
 * Json filter
 */
public abstract class JsonFilter<T>
{
   protected T object;
   protected Set<String> fields;
   
   /**
    * Create new filter
    * 
    * @param object
    * @param fields
    */
   protected JsonFilter(T object, Set<String> fields)
   {
      this.object = object;
      this.fields = fields;
   }

   /**
    * Do filtering
    * 
    * @return new object containing only requested fields
    */
   public abstract T filter();
   
   public static JsonFilter<?> createFilter(Object object, Set<String> fields)
   {
      if ((fields == null) || fields.isEmpty())
         return new JsonFilterDummy<Object>(object, fields);
      
      if (object instanceof JsonObject)
         return new JsonFilterJsonObject((JsonObject)object, fields);

      if (object instanceof JsonArray)
         return new JsonFilterJsonArray((JsonArray)object, fields);

      return new JsonFilterDummy<Object>(object, fields);
   }
}
