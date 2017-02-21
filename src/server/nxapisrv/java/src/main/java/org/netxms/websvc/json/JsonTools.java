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

import java.net.InetAddress;
import java.util.Date;
import org.netxms.base.InetAddressEx;
import org.netxms.base.annotations.Internal;
import org.netxms.client.MacAddress;
import org.netxms.websvc.json.adapters.DateAdapter;
import org.netxms.websvc.json.adapters.InetAddressAdapter;
import org.netxms.websvc.json.adapters.InetAddressExAdapter;
import org.netxms.websvc.json.adapters.MacAddressAdapter;
import com.google.gson.ExclusionStrategy;
import com.google.gson.FieldAttributes;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

/**
 * Collection of JSON tools
 */
public class JsonTools
{
   /**
    * Create JSON representation for given object
    * 
    * @param object object to serialize
    * @return JSON code
    */
   public static String jsonFromObject(Object object)
   {
      if (object instanceof JsonObject)
         return ((JsonObject)object).toString();
      if (object instanceof ResponseContainer)
         return ((ResponseContainer)object).toJson();
      
      GsonBuilder builder = new GsonBuilder();
      builder.setPrettyPrinting();  // FIXME: remove for production
      builder.registerTypeAdapter(Date.class, new DateAdapter());
      builder.registerTypeAdapter(InetAddress.class, new InetAddressAdapter());
      builder.registerTypeAdapter(InetAddressEx.class, new InetAddressExAdapter());
      builder.registerTypeAdapter(MacAddress.class, new MacAddressAdapter());
      builder.setExclusionStrategies(new ExclusionStrategy() {
         @Override
         public boolean shouldSkipField(FieldAttributes f)
         {
            return f.getAnnotation(Internal.class) != null;
         }
         
         @Override
         public boolean shouldSkipClass(Class<?> c)
         {
            return c.isAnnotationPresent(Internal.class);
         }
      });
      return builder.create().toJson(object);
   }
   
   /**
    * Create JsonObject from JSON code. Will return empty object if input string cannot be parsed.
    * 
    * @param json JSON code
    * @return Parsed JsonObject
    */
   public static JsonObject objectFromJson(String json)
   {
      try
      {
         JsonParser parser = new JsonParser();
         return parser.parse(json).getAsJsonObject();
      }
      catch(Exception e)
      {
         return new JsonObject();
      }
   }
}
