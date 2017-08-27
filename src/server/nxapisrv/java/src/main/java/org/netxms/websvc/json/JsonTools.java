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
import java.util.Set;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.base.annotations.Internal;
import org.netxms.websvc.json.adapters.DateAdapter;
import org.netxms.websvc.json.adapters.InetAddressAdapter;
import org.netxms.websvc.json.adapters.InetAddressExAdapter;
import org.netxms.websvc.json.adapters.MacAddressAdapter;
import com.google.gson.ExclusionStrategy;
import com.google.gson.FieldAttributes;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
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
   public static String jsonFromObject(Object object, Set<String> fields)
   {
      if ((object instanceof JsonObject) || 
          (object instanceof JsonArray) ||
          (object instanceof JSONObject) ||
          (object instanceof JSONArray))
         return JsonFilter.createFilter(object, fields).filter().toString();
      if (object instanceof ResponseContainer)
         return ((ResponseContainer)object).toJson(fields);
      
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
      if ((fields != null) && !fields.isEmpty())
      {
         return JsonFilter.createFilter(builder.create().toJsonTree(object), fields).filter().toString();
      }
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
   
   /**
    * Get JSON value as a string by key
    * 
    * @param data base JSON object
    * @param key search object key
    * @param defaultValue value in case searched object not found
    * @return JSON value as a string
    */
   public static String getStringFromJson(JSONObject data, String key, String defaultValue)
   {
      String value = defaultValue;
      try
      {
         value = data.getString(key);
      }
      catch(JSONException ex)
      {
      }
      
      return value;
   }
   
   /**
    * Get JSON value as an int by key
    * 
    * @param data base JSON object
    * @param key search object key
    * @param defaultValue value in case searched object not found
    * @return JSON value as an int
    */
   public static int getIntFromJson(JSONObject data, String key, int defaultValue)
   {
      int value = defaultValue;
      try
      {
         value = data.getInt(key);
      }
      catch(JSONException ex)
      {
      }
      
      return value;
   }
   
   /**
    * Get JSON value as a long by key
    * 
    * @param data base JSON object
    * @param key search object key
    * @param defaultValue value in case searched object not found
    * @return JSON value as a long
    */
   public static long getLongFromJson(JSONObject data, String key, long defaultValue)
   {
      long value = defaultValue;
      try
      {
         value = data.getLong(key);
      }
      catch(JSONException ex)
      {
      }
      
      return value;
   }
   
   /**
    * Get JSON value as an enum by key
    * 
    * @param data base JSON object
    * @param clazz enum class to convert to
    * @param key search object key
    * @param defaultValue value in case searched object not found
    * @return JSON value as an enum
    */
   public static <E extends Enum<E>> E getEnumFromJson(JSONObject data, Class<E> clazz, String key, E defaultValue)
   {
      E value = defaultValue;
      try
      {
         value = data.getEnum(clazz, key);
      }
      catch(JSONException ex)
      {
      }
      
      return value;
   }
   
   /**
    * Get JSON value as a JSONArray by key
    * 
    * @param data base JSON object
    * @param key search object key
    * @param defaultValue value in case searched object not found
    * @return JSON value as a JSONArray
    */
   public static JSONArray getJsonArrayFromJson(JSONObject data, String key, JSONArray defaultValue)
   {
      JSONArray value = defaultValue;
      try
      {
         value = data.getJSONArray(key);
      }
      catch(JSONException ex)
      {
      }
      
      return value;
   }
   
   /**
    * Get JSON value as a boolean by key
    * 
    * @param data base JSON object
    * @param key search object key
    * @param defaultValue value in case searched object not found
    * @return value as a boolean
    */
   public static boolean getBooleanFromJson(JSONObject data, String key, boolean defaultValue)
   {
      boolean value = defaultValue;
      try
      {
         value = data.getBoolean(key);
      }
      catch(JSONException ex)
      {
      }
      
      return value;
   }
}
