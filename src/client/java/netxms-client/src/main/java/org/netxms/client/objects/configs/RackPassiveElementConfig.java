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
package org.netxms.client.objects.configs;

import java.io.StringWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.List;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Rack attribute config
 */
@Root(name="rackPassiveElementConfig")
public class RackPassiveElementConfig
{
   @ElementList(required=true)
   private List<RackPassiveElementConfigEntry> entryList;
   
   /**
    * Create new rack attribute config
    */
   public RackPassiveElementConfig()
   {
      entryList = new ArrayList<RackPassiveElementConfigEntry>();
   }
   
   /**
    * Copy constructor for rack attribute config
    */
   public RackPassiveElementConfig(RackPassiveElementConfig src)
   {
      entryList = new ArrayList<RackPassiveElementConfigEntry>(src.entryList);
   }
   
   /**
    * Create rack attribute config from xml document
    * 
    * @param xml config
    * @return RackAttributeconfig
    * @throws Exception
    */
   public static RackPassiveElementConfig createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(RackPassiveElementConfig.class, xml);
   }
   
   /**
    * Create XML from configuration
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      Serializer serializer = new Persister();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }
   
   /**
    * Get rack attribute config entry list
    * 
    * @return entryList
    */
   public List<RackPassiveElementConfigEntry> getEntryList()
   {
      return entryList;
   }
   
   /**
    * Add new attribute
    * 
    * @param attribute to add
    */
   public void addEntry(RackPassiveElementConfigEntry attribute)
   {
      entryList.add(attribute);
   }
}
