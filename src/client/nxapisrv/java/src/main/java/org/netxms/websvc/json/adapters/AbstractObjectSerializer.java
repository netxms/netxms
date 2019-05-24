/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.websvc.json.JsonTools;
import com.google.gson.Gson;
import com.google.gson.JsonElement;
import com.google.gson.JsonSerializationContext;
import com.google.gson.JsonSerializer;

/**
 * Custom serializer for abstract object class
 */
public class AbstractObjectSerializer implements JsonSerializer<AbstractObject>
{
   private Gson gson = null;
   
   /**
    * Serialize element
    */
   @Override
   public JsonElement serialize(AbstractObject src, Type typeOfSrc, JsonSerializationContext context)
   {
      if (gson == null)
         gson = JsonTools.createGsonInstance(AbstractObjectSerializer.class);
      JsonElement json = gson.toJsonTree(src, typeOfSrc);
      json.getAsJsonObject().addProperty("objectClassName", src.getObjectClassName());
      return json;
   }
}
