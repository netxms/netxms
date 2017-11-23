/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.util.UUID;
import org.netxms.client.constants.ObjectStatus;

/**
 * Common interface for rack elements (anything that can be mounted in rack)
 */
public interface RackElement
{
   public static final int RACK_POSITION_FRONT = 0;
   public static final int RACK_POSITION_REAR = 1;
   public static final int RACK_POSITION_FILL = 2;
   
   /**
    * Get object ID
    * 
    * @return object ID
    */
   public long getObjectId();

   /**
    * Get object name
    * 
    * @return object name
    */
   public String getObjectName();

   /**
    * Get object status
    * 
    * @return object status
    */
   public ObjectStatus getStatus();
   
   /**
    * Get rack object ID
    * 
    * @return rack object ID
    */
   public long getRackId();

   /**
    * Get rack image
    * 
    * @return rack image
    */
   public UUID getRackImage();

   /**
    * Get position in rack
    * 
    * @return position in rack
    */
   public short getRackPosition();

   /**
    * Get device height in rack units
    * 
    * @return device height in rack units
    */
   public short getRackHeight();
   
   /**
    * Get orientation of object in rack
    * 
    * @return orientation of object in rack
    */
   public int getRackOrientation();
   
   /**
    * Set orientation of object in rack
    * 
    * @param rackOrientation to set
    */
   public void setRackOrientation(int rackOrientation);
}
