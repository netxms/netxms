/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Base64;
import java.util.Collection;
import java.util.List;
import java.util.Properties;
import java.util.UUID;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.netxms.client.NXCSession;
import org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * In-memory store for preferences.
 */
public class Memento
{
   private static final RGB DEFAULT_COLOR = new RGB(0, 0, 0);
   private static final Logger logger = LoggerFactory.getLogger(HistoricalGraphView.class);

   /**
    * Build full property name for server-specific properties
    *
    * @param baseName base property name
    * @param session communication session
    * @return full property name
    */
   public static String serverProperty(String baseName, NXCSession session)
   {
      return baseName + "$" + Long.toString(session.getServerId());
   }

   /**
    * Build full property name for server-specific properties
    *
    * @param baseName base property name
    * @return full property name
    */
   public static String serverProperty(String baseName)
   {
      return serverProperty(baseName, Registry.getSession());
   }

   protected Properties properties;
   protected Properties defaultValues;

   /**
    * Default constructor
    */
   public Memento()
   {
      defaultValues = new Properties();
      properties = new Properties(defaultValues);
   }

   /**
    * Called when property changed. Default implementation does nothing.
    *
    * @param property property name
    * @param oldValue old value or null
    * @param newValue new value or null
    */
   protected void onPropertyChange(String property, String oldValue, String newValue)
   {
   }

   /**
    * Get property as string.
    * 
    * @param name property name
    * @return property value or null if not found
    */
   public String getAsString(String name)
   {
      return properties.getProperty(name);
   }

   /**
    * Get property as string.
    * 
    * @param name property name
    * @param defaultValue default value
    * @return property value or default value if not found
    */
   public String getAsString(String name, String defaultValue)
   {
      return properties.getProperty(name, defaultValue);
   }

   /**
    * Get property as boolean.
    * 
    * @param name property name
    * @param defaultValue default value
    * @return property value or default value if not found
    */
   public boolean getAsBoolean(String name, boolean defaultValue)
   {
      String v = properties.getProperty(name);
      return (v != null) ? Boolean.parseBoolean(v) : defaultValue;
   }

   /**
    * Get property as integer.
    * 
    * @param name property name
    * @param defaultValue default value
    * @return property value or default value if not found or cannot be interpreted as integer
    */
   public int getAsInteger(String name, int defaultValue)
   {
      String v = properties.getProperty(name);
      if (v == null)
         return defaultValue;
      try
      {
         return Integer.parseInt(v);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }

   /**
    * Get property as long integer.
    * 
    * @param name property name
    * @param defaultValue default value
    * @return property value or default value if not found or cannot be interpreted as long integer
    */
   public long getAsLong(String name, long defaultValue)
   {
      String v = properties.getProperty(name);
      if (v == null)
         return defaultValue;
      try
      {
         return Long.parseLong(v);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }

   /**
    * Get property as floating point number.
    * 
    * @param name property name
    * @param defaultValue default value
    * @return property value or default value if not found or cannot be interpreted as floating point number
    */
   public double getAsDouble(String name, double defaultValue)
   {
      String v = properties.getProperty(name);
      if (v == null)
         return defaultValue;
      try
      {
         return Double.parseDouble(v);
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }

   /**
    * Get property as point object.
    * 
    * @param name property name
    * @param defaultValue default value
    * @return property value or default value if not found or cannot be interpreted as a point
    */
   public Point getAsPoint(String name, Point defaultValue)
   {
      String v = properties.getProperty(name);
      if (v == null)
         return defaultValue;

      String[] parts = v.split(",");
      if (parts.length != 2)
         return defaultValue;

      try
      {
         return new Point(Integer.parseInt(parts[0]), Integer.parseInt(parts[1]));
      }
      catch(NumberFormatException e)
      {
         return defaultValue;
      }
   }

   /**
    * Get property as point object.
    * 
    * @param name property name
    * @param defaultX default value for point's X value
    * @param defaultY default value for point's Y value
    * @return property value or default value if not found or cannot be interpreted as a point
    */
   public Point getAsPoint(String name, int defaultX, int defaultY)
   {
      String v = properties.getProperty(name);
      if (v == null)
         return new Point(defaultX, defaultY);

      String[] parts = v.split(",");
      if (parts.length != 2)
         return new Point(defaultX, defaultY);

      try
      {
         return new Point(Integer.parseInt(parts[0]), Integer.parseInt(parts[1]));
      }
      catch(NumberFormatException e)
      {
         return new Point(defaultX, defaultY);
      }
   }

   /**
    * Get property as list of strings
    * 
    * @param name property name
    * @return list of strings (empty list if property was not set)
    */
   public List<String> getAsStringList(String name)
   {
      int count = getAsInteger(name + ".Count", 0);
      List<String> list = new ArrayList<String>(count);
      for(int i = 0; i < count; i++)
         list.add(getAsString(name + "." + Integer.toString(i), ""));
      return list;
   }

   /**
    * Get property as array of strings
    * 
    * @param name property name
    * @return list of strings (empty array if property was not set)
    */
   public String[] getAsStringArray(String name)
   {
      int count = getAsInteger(name + ".Count", 0);
      String[] list = new String[count];
      for(int i = 0; i < count; i++)
         list[i] = getAsString(name + "." + Integer.toString(i), "");
      return list;
   }

   /**
    * Get property as color definition.
    * 
    * @param name property name
    * @param defaultValue default value to be returned if property does not exist or cannot be parsed.
    * @return property value as color definition or default value
    */
   public RGB getAsColor(String name, RGB defaultValue)
   {
      String v = properties.getProperty(name);
      if (v == null)
         return defaultValue;

      String[] parts = v.split(",");
      if (parts.length != 3)
         return defaultValue;

      try
      {
         return new RGB(Integer.parseInt(parts[0]), Integer.parseInt(parts[1]), Integer.parseInt(parts[2]));
      }
      catch(Exception e)
      {
         return defaultValue;
      }
   }

   /**
    * Get property as color definition.
    * 
    * @param name property name
    * @return property value as color definition or default color value
    */
   public RGB getAsColor(String name)
   {
      return getAsColor(name, DEFAULT_COLOR);
   }

   /**
    * Get property as memento object.
    * 
    * @param name property name
    * @return property value as memento object
    */
   public Memento getAsMemento(String name)
   {
      Memento memento = new Memento();
      String value = getAsString(name);
      if ((value != null) && !value.isEmpty())
      {
         memento.deserialize(value);
      }
      return memento;
   }

   /**
    * Get parameter as UUID
    * 
    * @param name property name
    * @return property value as UUId object
    */
   public UUID getAsUUID(String name)
   {
      String value = getAsString(name);
      if ((value != null) && !value.isEmpty())
      {
         try
         {
            return UUID.fromString(value);
         }
         catch (IllegalArgumentException e)
         {
            logger.error("Failed to parse UUID", e);
            return null;
         }
      }
      return null;
   }

   /**
    * Set property.
    * 
    * @param name property name
    * @param value new property value
    */
   public void set(String name, String value)
   {
      String oldValue = properties.getProperty(name);
      properties.setProperty(name, value);
      onPropertyChange(name, oldValue, value);
   }

   /**
    * Set property.
    * 
    * @param name property name
    * @param value new property value
    */
   public void set(String name, boolean value)
   {
      set(name, Boolean.toString(value));
   }

   /**
    * Set property.
    * 
    * @param name property name
    * @param value new property value
    */
   public void set(String name, int value)
   {
      set(name, Integer.toString(value));
   }

   /**
    * Set property.
    *
    * @param name property name
    * @param value new property value
    */
   public void set(String name, long value)
   {
      set(name, Long.toString(value));
   }

   /**
    * Set property.
    *
    * @param name property name
    * @param value new property value
    */
   public void set(String name, double value)
   {
      set(name, Double.toString(value));
   }

   /**
    * Set property.
    *
    * @param name property name
    * @param value new property value
    */
   public void set(String name, Point value)
   {
      set(name, Integer.toString(value.x) + "," + Integer.toString(value.y));
   }

   /**
    * Set property.
    *
    * @param name property name
    * @param value new property value
    */
   public <T> void set(String name, Collection<T> value)
   {
      properties.setProperty(name + ".Count", Integer.toString(value.size()));
      int index = 0;
      for(T s : value)
         properties.setProperty(name + "." + Integer.toString(index++), s.toString());
      onPropertyChange(name, null, null);
   }

   /**
    * Set property. Will serialize provided memento object and store it as string.
    *
    * @param name property name
    * @param value new property value
    */
   public void set(String name, Memento value)
   {
      set(name, value.serialize());
   }

   /**
    * Set property.
    *
    * @param name property name
    * @param value new property value
    */
   public void set(String name, RGB value)
   {
      set(name, Integer.toString(value.red) + "," + Integer.toString(value.green) + "," + Integer.toString(value.blue));
   }

   /**
    * Set property.
    *
    * @param name property name
    * @param value new property value
    */
   public void set(String name, UUID value)
   {
      set(name, value.toString());
   }

   /**
    * Set property default value.
    *
    * @param name property name
    * @param value default value
    */
   public void setDefault(String name, String value)
   {
      defaultValues.setProperty(name, value);
   }

   /**
    * Set property default value.
    *
    * @param name property name
    * @param value default value
    */
   public void setDefault(String name, boolean value)
   {
      setDefault(name, Boolean.toString(value));
   }

   /**
    * Set property default value.
    *
    * @param name property name
    * @param value default value
    */
   public void setDefault(String name, int value)
   {
      setDefault(name, Integer.toString(value));
   }

   /**
    * Set property default value.
    *
    * @param name property name
    * @param value default value
    */
   public void setDefault(String name, long value)
   {
      setDefault(name, Long.toString(value));
   }

   /**
    * Set property default value.
    *
    * @param name property name
    * @param value default value
    */
   public void setDefault(String name, Point value)
   {
      setDefault(name, Integer.toString(value.x) + "," + Integer.toString(value.y));
   }

   /**
    * Set property default value.
    *
    * @param name property name
    * @param value default value
    */
   public void setDefault(String name, Collection<String> value)
   {
      defaultValues.setProperty(name + ".Count", Integer.toString(value.size()));
      int index = 0;
      for(String s : value)
         defaultValues.setProperty(name + "." + Integer.toString(index), s);
   }

   /**
    * Set property default value.
    *
    * @param name property name
    * @param value default value
    */
   public void setDefault(String name, RGB value)
   {
      setDefault(name, Integer.toString(value.red) + "," + Integer.toString(value.green) + "," + Integer.toString(value.blue));
   }

   /**
    * Remove property.
    *
    * @param name property name
    */
   public void remove(String name)
   {
      properties.remove(name);
   }

   /**
    * Serialize this memento to base-64 encoded string.
    *
    * @return base-64 encoded string
    */
   protected String serialize()
   {
      try (ByteArrayOutputStream out = new ByteArrayOutputStream(8192))
      {
         properties.store(out, "");
         return Base64.getEncoder().encodeToString(out.toByteArray());
      }
      catch(IOException e)
      {
         return null;
      }
   }

   /**
    * Deserialize this memento from base-64 encoded string. Will clear any existing keys before deserialization.
    *
    * @param encoded base64 encoded content
    */
   protected void deserialize(String encoded)
   {
      try
      {
         byte[] bytes = Base64.getDecoder().decode(encoded);
         InputStream in = new ByteArrayInputStream(bytes);
         properties.clear();
         properties.load(in);
      }
      catch(Exception e)
      {
      }
   }
}
