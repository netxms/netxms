/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.client.xml;

import java.io.StringWriter;
import java.io.Writer;
import java.util.UUID;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.convert.AnnotationStrategy;
import org.simpleframework.xml.core.Persister;
import org.simpleframework.xml.filter.Filter;
import org.simpleframework.xml.transform.RegistryMatcher;
import org.simpleframework.xml.transform.Transform;

/**
 * Tools for XML conversion
 */
public final class XMLTools
{
   private static RegistryMatcher matcher = new RegistryMatcher();
   static
   {
      try
      {
         matcher.bind(UUID.class, new UUIDTransform());
      }
      catch(Exception e)
      {
      }
   }

   /**
    * Dummy filter to prevent expansion of ${name} in XML data
    */
   private static Filter filter = new Filter() {
      @Override
      public String replace(String text)
      {
         return null;
      }
   };

   /**
    * Register global transform for given data type.
    *
    * @param type data type (class)
    * @param transform transform
    */
   public static void registerTransform(Class<?> type, Transform<?> transform)
   {
      matcher.bind(type, transform);
   }

   /**
    * Create serializer with registered transforms
    * 
    * @return serializer with registered transforms
    * @throws Exception on XML library failures
    */
   public static Serializer createSerializer() throws Exception
   {
      return new Persister(new AnnotationStrategy(), filter, matcher);
   }

   /**
    * Create object from XML document.
    *
    * @param <T> object class
    * @param c object class
    * @param xml XML document
    * @return deserialized object
    * @throws Exception on error
    */
   public static <T> T createFromXml(Class<T> c, String xml) throws Exception
   {
      Serializer serializer = createSerializer();
      return serializer.read(c, xml, false);
   }

   /**
    * Serialize given object to XML.
    *
    * @param object object to serialize
    * @return serialized object
    * @throws Exception on error
    */
   public static String serialize(Object object) throws Exception
   {
      Serializer serializer = createSerializer();
      Writer writer = new StringWriter();
      serializer.write(object, writer);
      return writer.toString();
   }
}
