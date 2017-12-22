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
import org.netxms.client.constants.RackOrientation;

/**
 * Common interface for rack elements (anything that can be mounted in rack)
 */
public interface RackElement
{
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
    * Get front rack image
    * 
    * @return front rack image
    */
   public UUID getFrontRackImage();
   
   /**
    * Get rear rack image
    * 
    * @return rear rack image
    */
   public UUID getRearRackImage();

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
   public RackOrientation getRackOrientation();
}
