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
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.configs.ChassisPlacement;
import org.netxms.client.objects.interfaces.HardwareEntity;

/**
 * Chassis object
 */
public class Chassis extends DataCollectionTarget implements HardwareEntity
{
   public static final int CHF_BIND_UNDER_CONTROLLER = 0x00010000;

   protected long controllerId;
   protected long rackId;
   protected UUID rackImageFront;
   protected UUID rackImageRear;
   protected short rackPosition;
   protected short rackHeight;
   protected RackOrientation rackOrientation;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public Chassis(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      controllerId = msg.getFieldAsInt64(NXCPCodes.VID_CONTROLLER_ID);
      rackId = msg.getFieldAsInt64(NXCPCodes.VID_PHYSICAL_CONTAINER_ID);
      rackImageFront = msg.getFieldAsUUID(NXCPCodes.VID_RACK_IMAGE_FRONT);
      rackImageRear = msg.getFieldAsUUID(NXCPCodes.VID_RACK_IMAGE_REAR);
      rackPosition = msg.getFieldAsInt16(NXCPCodes.VID_RACK_POSITION);
      rackHeight = msg.getFieldAsInt16(NXCPCodes.VID_RACK_HEIGHT);
      rackOrientation = RackOrientation.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_RACK_ORIENTATION));
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "Chassis";
   }

   /**
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

   /**
    * Get object flags.
    *
    * @return object flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * @see org.netxms.client.objects.interfaces.HardwareEntity#getPhysicalContainerId()
    */
   @Override
   public long getPhysicalContainerId()
   {
      return rackId;
   }

   /**
    * @see org.netxms.client.objects.interfaces.HardwareEntity#getFrontRackImage()
    */
   @Override
   public UUID getFrontRackImage()
   {
      return rackImageFront;
   }
   
   /**
    * @see org.netxms.client.objects.interfaces.HardwareEntity#getRearRackImage()
    */
   @Override
   public UUID getRearRackImage()
   {
      return rackImageRear;
   }

   /**
    * @see org.netxms.client.objects.interfaces.HardwareEntity#getRackPosition()
    */
   @Override
   public short getRackPosition()
   {
      return rackPosition;
   }

   /**
    * @see org.netxms.client.objects.interfaces.HardwareEntity#getRackHeight()
    */
   @Override
   public short getRackHeight()
   {
      return rackHeight;
   }

   /**
    * @see org.netxms.client.objects.interfaces.HardwareEntity#getRackOrientation()
    */
   @Override
   public RackOrientation getRackOrientation()
   {
      return rackOrientation;
   }

   /**
    * @see org.netxms.client.objects.interfaces.HardwareEntity#getChassisPlacement()
    */
   @Override
   public ChassisPlacement getChassisPlacement()
   {
      return null;
   }
}
