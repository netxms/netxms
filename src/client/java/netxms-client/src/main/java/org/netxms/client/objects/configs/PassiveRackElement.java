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
import org.netxms.client.constants.RackElementType;
import org.netxms.client.constants.RackOrientation;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Rack element configuration entry
 */
@Root(name="element")
public class PassiveRackElement
{
   @Element(required=false)
   public String name;

   @Element(required=true)
   public RackElementType type;
   
   @Element(required=true)
   public int position;

   @Element(required=true)
   public RackOrientation orientation;

   /**
    * Create empty rack attribute entry
    */
   public PassiveRackElement()
   {
      name = "";
      type = RackElementType.FILLER_PANEL;
      position = 0;
      orientation = RackOrientation.FRONT;
   }
   
   /**
    * Create rack attribute from config entry
    * 
    * @param xml config
    * @return attribute
    * @throws Exception
    */
   public static PassiveRackElement createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(PassiveRackElement.class, xml);
   }
   
   /**
    * Create XML from configuration entry
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
    * Set element type
    * 
    * @param type to set
    */
   public void setType(RackElementType type)
   {
      this.type = type;
   }
   
   /**
    * Get element type
    * 
    * @return element type
    */
   public RackElementType getType()
   {
      return type;
   }
   
   /**
    * Set attribute position
    * 
    * @param position to set
    */
   public void setPosition(int position)
   {
      this.position = position;
   }
   
   /**
    * Get attribute position in rack
    * 
    * @return position
    */
   public int getPosition()
   {
      return position;
   }
   
   /**
    * Set attribute orientation
    * 
    * @param orientation to set
    */
   public void setOrientation(RackOrientation orientation)
   {
      this.orientation = orientation;
   }
   
   /**
    * Get orientation
    * 
    * @return orientation
    */
   public RackOrientation getOrientation()
   {
      return orientation;
   }
   
   /**
    * Get element name
    * 
    * @return element name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Set element name
    * 
    * @param name new element name
    */
   public void setName(String name)
   {
      this.name = name;
   }
}
