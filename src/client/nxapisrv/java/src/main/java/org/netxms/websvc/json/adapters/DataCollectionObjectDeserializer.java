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
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.websvc.json.JsonTools;
import com.google.gson.Gson;
import com.google.gson.JsonDeserializationContext;
import com.google.gson.JsonDeserializer;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParseException;
import com.google.gson.JsonPrimitive;

/**
 * Deserializer for classes derived from DataCollectionObject
 */
public class DataCollectionObjectDeserializer implements JsonDeserializer<DataCollectionObject>
{
   private Gson gson;

   /**
    * @see com.google.gson.JsonDeserializer#deserialize(com.google.gson.JsonElement, java.lang.reflect.Type, com.google.gson.JsonDeserializationContext)
    */
   @Override
   public DataCollectionObject deserialize(JsonElement src, Type typeOfSrc, JsonDeserializationContext context) throws JsonParseException
   {
      JsonObject jsonObject = src.getAsJsonObject();
      JsonPrimitive type = jsonObject.getAsJsonPrimitive("valueType");
      if (type == null)
         throw new JsonParseException("Missing mandatory attribute \"valueType\"");
      
      if (gson == null)
         gson = JsonTools.createGsonInstance(DataCollectionObjectDeserializer.class);

      String stype = type.getAsString();
      if ("single".equals(stype))
         return gson.fromJson(src, DataCollectionItem.class);
      if ("table".equals(stype))
         return gson.fromJson(src, DataCollectionTable.class);

      throw new JsonParseException("Invalid value of mandatory attribute \"valueType\"");
   }
}
