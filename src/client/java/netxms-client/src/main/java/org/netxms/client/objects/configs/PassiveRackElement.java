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

import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.RackElementType;
import org.netxms.client.constants.RackOrientation;

/**
 * Rack element configuration entry
 */
public class PassiveRackElement
{
   public long id;
   public String name;
   public RackElementType type;
   public int position;
   public RackOrientation orientation;
   public int portCount;

   /**
    * Create empty rack attribute entry
    */
   public PassiveRackElement()
   {
      id = 0;
      name = "";
      type = RackElementType.PATCH_PANEL;
      position = 0;
      orientation = RackOrientation.FILL;
      portCount = 0;
   }

   /**
    * Rack passive element constructor 
    * 
    * @param msg message from server
    * @param base base id for creation
    */
   public PassiveRackElement(NXCPMessage msg, long base)
   {
      id = msg.getFieldAsInt32(base++);
      name = msg.getFieldAsString(base++);
      type = RackElementType.getByValue(msg.getFieldAsInt32(base++));
      position = msg.getFieldAsInt32(base++);
      orientation = RackOrientation.getByValue(msg.getFieldAsInt32(base++));
      portCount = msg.getFieldAsInt32(base++);
   }


   /**
    * Rack passive element copy constructor 
    * @param element element to copy
    */
   public PassiveRackElement(PassiveRackElement element)
   {
      id = element.id;
      name = element.name;
      type = element.type;
      position = element.position;
      orientation = element.orientation;
   }

   /**
    * Fill message with passive rack element data 
    * 
    * @param msg message to fill
    * @param base base id to start from
    */
   public void fillMessage(NXCPMessage msg, long base)
   {
      msg.setFieldInt32(base++, (int)id);
      msg.setField(base++, name);
      msg.setFieldInt32(base++, type.getValue());
      msg.setFieldInt32(base++, position);
      msg.setFieldInt32(base++, orientation.getValue());
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

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @param id the id to set
    */
   public void setId(long id)
   {
      this.id = id;
   }

   /**
    * @return the portCount
    */
   public int getPortCount()
   {
      return portCount;
   }

   /**
    * @param portCount the portCount to set
    */
   public void setPortCount(int portCount)
   {
      this.portCount = portCount;
   }

   /**
    * To string method for object
    */
   public String toString()
   {
      return orientation.toString() + " " + position + " (" + name + ")";
   }
}
