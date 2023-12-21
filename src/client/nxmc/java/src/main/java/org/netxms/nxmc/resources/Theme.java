/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.resources;

import java.io.File;
import java.io.FileWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.swt.graphics.RGB;
import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.tools.RGBTransform;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.ElementMap;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;

/**
 * UI theme
 */
@Root(name = "theme")
public class Theme
{
   static
   {
      XMLTools.registerTransform(RGB.class, new RGBTransform());
   }

   @Attribute
   protected String name;

   @ElementMap
   protected Map<String, ThemeElement> elements = new HashMap<String, ThemeElement>();

   /**
    * Load theme object from file.
    *
    * @param file XMLS file with theme definition
    * @return theme object
    * @throws Exception on I/O or XML parsing failure
    */
   public static Theme load(File file) throws Exception
   {
      Serializer serializer = XMLTools.createSerializer();
      return serializer.read(Theme.class, file, false);
   }

   /**
    * Create new theme object.
    */
   public Theme()
   {
      name = "Unnamed";
   }

   /**
    * Create new theme object with given name.
    *
    * @param name theme name
    */
   public Theme(String name)
   {
      this.name = name;
   }

   /**
    * Create copy of given theme object with new name.
    *
    * @param name theme name
    * @param src source theme
    */
   public Theme(String name, Theme src)
   {
      this.name = name;
      this.elements.putAll(src.elements);
   }

   /**
    * Save theme object to XML file
    *
    * @param file XML file
    * @throws Exception on I/O on serialization failure
    */
   public void save(File file) throws Exception
   {
      Serializer serializer = XMLTools.createSerializer();
      Writer writer = new FileWriter(file);
      try
      {
         serializer.write(this, writer);
      }
      finally
      {
         writer.close();
      }
   }

   /**
    * Get theme name
    *
    * @return theme name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Set theme name
    *
    * @param name new theme name
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * Get theme element by tag
    *
    * @param tag element tag
    * @return theme element or null
    */
   public ThemeElement getElement(String tag)
   {
      return elements.get(tag);
   }

   /**
    * Set element for given tag.
    *
    * @param tag element tag
    * @param element new element
    */
   public void setElement(String tag, ThemeElement element)
   {
      elements.put(tag, element);
   }

   /**
    * Get all color tags in this theme
    *
    * @return all color tags in this theme
    */
   public List<String> getTags()
   {
      return new ArrayList<String>(elements.keySet());
   }

   /**
    * Set elements that are missing comparing to provided source theme.
    *
    * @param src source theme
    */
   public void setMissingElements(Theme src)
   {
      for(String tag : src.elements.keySet())
         elements.putIfAbsent(tag, src.elements.get(tag));
   }
}
