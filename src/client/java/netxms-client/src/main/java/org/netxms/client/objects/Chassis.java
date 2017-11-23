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
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Chassis object
 */
public class Chassis extends DataCollectionTarget implements RackElement
{
   protected long controllerId;
   protected long rackId;
   protected UUID rackImage;
   protected short rackPosition;
   protected short rackHeight;
   protected int rackOrientation;

   /**
    * @param msg
    * @param session
    */
   public Chassis(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      controllerId = msg.getFieldAsInt64(NXCPCodes.VID_CONTROLLER_ID);
      rackId = msg.getFieldAsInt64(NXCPCodes.VID_RACK_ID);
      rackImage = msg.getFieldAsUUID(NXCPCodes.VID_RACK_IMAGE);
      rackPosition = msg.getFieldAsInt16(NXCPCodes.VID_RACK_POSITION);
      rackHeight = msg.getFieldAsInt16(NXCPCodes.VID_RACK_HEIGHT);
      rackOrientation = msg.getFieldAsInt16(NXCPCodes.VID_RACK_ORIENTATION);
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "Chassis";
   }
   
   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#isAlarmsVisible()
    */
   @Override
   public boolean isAlarmsVisible()
   {
      return true;
   }

   /**
    * @return the controllerId
    */
   public long getControllerId()
   {
      return controllerId;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRackId()
    */
   @Override
   public long getRackId()
   {
      return rackId;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRackImage()
    */
   @Override
   public UUID getRackImage()
   {
      return rackImage;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRackPosition()
    */
   @Override
   public short getRackPosition()
   {
      return rackPosition;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRackHeight()
    */
   @Override
   public short getRackHeight()
   {
      return rackHeight;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRackOrientation()
    */
   @Override
   public int getRackOrientation()
   {
      return rackOrientation;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#setRackOrientation(int)
    */
   @Override
   public void setRackOrientation(int rackOrientation)
   {
      this.rackOrientation = rackOrientation;
   }
}
