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

import java.util.UUID;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;
import org.netxms.client.constants.PassiveRackElementType;
import org.netxms.client.constants.RackOrientation;

/**
 * Rack element configuration entry
 */
public class PassiveRackElement
{
   public long id;
   public String name;
   public PassiveRackElementType type;
   public int position;
   public int height;
   public RackOrientation orientation;
   public int portCount;
   public UUID frontImage;
   public UUID rearImage;

   /**
    * Create empty rack attribute entry
    */
   public PassiveRackElement()
   {
      id = 0;
      name = "";
      type = PassiveRackElementType.FILLER_PANEL;
      position = 0;
      height = 1;
      orientation = RackOrientation.FILL;
      portCount = 0;
      frontImage = NXCommon.EMPTY_GUID;
      rearImage = NXCommon.EMPTY_GUID;
   }

   /**
    * Rack passive element constructor
    * 
    * @param msg message from server
    * @param baseId base field ID
    */
   public PassiveRackElement(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId++);
      name = msg.getFieldAsString(baseId++);
      type = PassiveRackElementType.getByValue(msg.getFieldAsInt32(baseId++));
      position = msg.getFieldAsInt32(baseId++);
      height = msg.getFieldAsInt32(baseId++);
      orientation = RackOrientation.getByValue(msg.getFieldAsInt32(baseId++));
      portCount = msg.getFieldAsInt32(baseId++);
      frontImage = msg.getFieldAsUUID(baseId++);
      rearImage = msg.getFieldAsUUID(baseId++);
   }

   /**
    * Rack passive element copy constructor
    *
    * @param src element to copy
    */
   public PassiveRackElement(PassiveRackElement src)
   {
      id = src.id;
      name = src.name;
      type = src.type;
      position = src.position;
      height = src.height;
      orientation = src.orientation;
      portCount = src.portCount;
      frontImage = src.frontImage;
      rearImage = src.rearImage;
   }

   /**
    * Fill message with passive rack element data 
    * 
    * @param msg message to fill
    * @param baseId base id to start from
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setFieldInt32(baseId++, (int)id);
      msg.setField(baseId++, name);
      msg.setFieldInt16(baseId++, type.getValue());
      msg.setFieldInt16(baseId++, position);
      msg.setFieldInt16(baseId++, height);
      msg.setFieldInt16(baseId++, orientation.getValue());
      msg.setFieldInt16(baseId++, portCount);
      msg.setField(baseId++, frontImage);
      msg.setField(baseId++, rearImage);
   }

   /**
    * Set element type
    * 
    * @param type to set
    */
   public void setType(PassiveRackElementType type)
   {
      this.type = type;
   }

   /**
    * Get element type
    * 
    * @return element type
    */
   public PassiveRackElementType getType()
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
    * @return the height
    */
   public int getHeight()
   {
      return height;
   }

   /**
    * @param height the height to set
    */
   public void setHeight(int height)
   {
      this.height = height;
   }

   /**
    * @return the frontImage
    */
   public UUID getFrontImage()
   {
      return frontImage;
   }

   /**
    * @param frontImage the frontImage to set
    */
   public void setFrontImage(UUID frontImage)
   {
      this.frontImage = frontImage;
   }

   /**
    * @return the rearImage
    */
   public UUID getRearImage()
   {
      return rearImage;
   }

   /**
    * @param rearImage the rearImage to set
    */
   public void setRearImage(UUID rearImage)
   {
      this.rearImage = rearImage;
   }

   /**
    * To string method for object
    */
   public String toString()
   {
      return orientation.toString() + " " + position + " (" + name + ")";
   }
}
