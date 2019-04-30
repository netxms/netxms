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
import org.netxms.client.constants.HardwareComponentCategory;

/**
 * Hardware component information
 */
public class HardwareComponent
{
   private HardwareComponentCategory category;
   private int index;
   private long capacity;
   private String type;
   private String vendor;
   private String model;
   private String partNumber;
   private String serialNumber;
   private String location;
   private String description;
   
   /**
    * Create hardware component from NXCPMessage
    * 
    * @param msg the NXCPMessage
    * @param baseId the base id
    */
   public HardwareComponent(NXCPMessage msg, long baseId)
   {
      category = HardwareComponentCategory.getByValue(msg.getFieldAsInt32(baseId));
      index = msg.getFieldAsInt32(baseId + 1);
      type = msg.getFieldAsString(baseId + 2);
      vendor = msg.getFieldAsString(baseId + 3);
      model = msg.getFieldAsString(baseId + 4);
      location = msg.getFieldAsString(baseId + 5);
      capacity = msg.getFieldAsInt64(baseId + 6);
      partNumber = msg.getFieldAsString(baseId + 7);
      serialNumber = msg.getFieldAsString(baseId + 8);
      description = msg.getFieldAsString(baseId + 9);
   }
   
   /**
    * Get component category.
    * 
    * @return component category
    */
   public HardwareComponentCategory getCategory()
   {
      return category;
   }

   /**
    * Get component index (handle).
    * 
    * @return component index
    */
   public int getIndex()
   {
      return index;
   }
   
   /**
    * Get component type.
    * 
    * @return component type
    */
   public String getType()
   {
      return type;
   }
   
   /**
    * Get component's vendor name.
    * 
    * @return component's vendor name
    */
   public String getVendor()
   {
      return vendor;
   }
   
   /**
    * Get component's mode name.
    * 
    * @return component's mode name
    */
   public String getModel()
   {
      return model;
   }
   
   /**
    * Get component's capacity. Actual meaning depends on component category:
    * <p>Capacity in bytes for storage and memory devices
    * <p>Speed in kHz for processors
    * <p>Capacity in mWh for batteries
    * 
    * @return component's capacity
    */
   public long getCapacity()
   {
      return capacity;
   }
   
   /**
    * Get component's serial number.
    * 
    * @return component's serial number
    */
   public String getSerialNumber()
   {
      return serialNumber;
   }

   /**
    * Get component's part number.
    * 
    * @return component's part number
    */
   public String getPartNumber()
   {
      return partNumber;
   }

   /**
    * Get component's location within chassis or parent component.
    * 
    * @return component's location
    */
   public String getLocation()
   {
      return location;
   }

   /**
    * Get category specific component description.
    *  
    * @return component's description
    */
   public String getDescription()
   {
      return description;
   }
}
