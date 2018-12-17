/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * Hardware component information
 */
public class HardwareComponent
{
   private String type;
   private int index;
   private String vendor;
   private String model;
   private int capacity;
   private String serial;
   
   /**
    * Create hardware component from NXCPMessage
    * 
    * @param msg the NXCPMessage
    * @param baseId the base id
    */
   public HardwareComponent(NXCPMessage msg, long baseId)
   {
      type = msg.getFieldAsString(baseId);
      index = msg.getFieldAsInt32(baseId + 1);
      vendor = msg.getFieldAsString(baseId + 2);
      model = msg.getFieldAsString(baseId + 3);
      capacity = msg.getFieldAsInt32(baseId + 4);
      serial = msg.getFieldAsString(baseId + 5);
   }
   
   /**
    * @return the type
    */
   public String getType()
   {
      return type;
   }
   
   /**
    * @return the index
    */
   public int getIndex()
   {
      return index;
   }
   
   /**
    * @return the vendor
    */
   public String getVendor()
   {
      return vendor;
   }
   
   /**
    * @return the model
    */
   public String getModel()
   {
      return model;
   }
   
   /**
    * @return capacity
    */
   public int getCapacity()
   {
      return capacity;
   }
   
   /**
    * @return serial
    */
   public String getSerial()
   {
      return serial;
   }
}
