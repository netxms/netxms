/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.websvc.json.adapters;

import java.lang.reflect.Type;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.websvc.json.JsonTools;
import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonSerializationContext;
import com.google.gson.JsonSerializer;

/**
 * Serializer for data collection objects
 */
public class DataCollectionObjectSerializer implements JsonSerializer<DataCollectionObject>
{
   private Gson gson;

   /**
    * @see com.google.gson.JsonSerializer#serialize(java.lang.Object, java.lang.reflect.Type,
    *      com.google.gson.JsonSerializationContext)
    */
   @Override
   public JsonElement serialize(DataCollectionObject src, Type typeOfSrc, JsonSerializationContext context)
   {
      if (gson == null)
         gson = JsonTools.createGsonInstance(DataCollectionObjectSerializer.class);
      JsonElement json = gson.toJsonTree(src, typeOfSrc);
      json.getAsJsonObject().addProperty("valueType", (src instanceof DataCollectionItem) ? "single" : "table");
      return json;
   }
}
