/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import java.io.StringWriter;
import java.io.Writer;
import org.netxms.client.xml.XMLTools;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;

/**
 * File delivery policy
 */
@Root(name="FileDeliveryPolicy", strict=false)
public class FileDeliveryPolicy
{
   @ElementArray(required = false)
   public PathElement[] elements = new PathElement[0];

   /**
    * Create support application policy from XML
    * 
    * @param xml configuration
    * @return attribute support application policy
    * @throws Exception if deserialization fails
    */
   public static FileDeliveryPolicy createFromXml(final String xml) throws Exception
   {
      Serializer serializer = XMLTools.createSerializer();
      FileDeliveryPolicy policy = serializer.read(FileDeliveryPolicy.class, xml);
      for(PathElement e : policy.elements)
         e.updateParentReference(null);
      return policy;
   }

   /**
    * Create XML from configuration entry
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      Serializer serializer = XMLTools.createSerializer();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }
}
