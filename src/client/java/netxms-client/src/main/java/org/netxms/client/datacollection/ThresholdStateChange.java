/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Information about threshold state change
 */
public class ThresholdStateChange
{
   long objectId;
   long dciId;
   long thresholdId;
   String instance;
   boolean activated;
   
   /**
    * Create state change object from NXCP message.
    * 
    * @param msg NXCP message
    */
   public ThresholdStateChange(NXCPMessage msg)
   {
      objectId = msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID);
      dciId = msg.getFieldAsInt64(NXCPCodes.VID_DCI_ID);
      thresholdId = msg.getFieldAsInt64(NXCPCodes.VID_THRESHOLD_ID);
      instance = msg.getFieldAsString(NXCPCodes.VID_INSTANCE);
      activated = msg.getFieldAsBoolean(NXCPCodes.VID_STATE);
   }

   /**
    * Get object ID.
    * 
    * @return object ID
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * Get DCI ID.
    * 
    * @return DCI ID
    */
   public long getDciId()
   {
      return dciId;
   }

   /**
    * Get threshold ID.
    * 
    * @return threshold ID
    */
   public long getThresholdId()
   {
      return thresholdId;
   }

   /**
    * Get instance for table DCI threshold (will return null for single value DCI).
    *  
    * @return instance
    */
   public String getInstance()
   {
      return instance;
   }

   /**
    * Check if threshold was activated.
    * 
    * @return true if threshold was activated
    */
   public boolean isActivated()
   {
      return activated;
   }

   
}
