/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.CoolingZoneType;

/**
 * Cooling zone object - cooling plant or thermal zone within a facility
 */
public class CoolingZone extends DataCollectionContainer
{
   private CoolingZoneType zoneType;
   private int ratedCapacity;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public CoolingZone(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      zoneType = CoolingZoneType.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_ZONE_TYPE));
      ratedCapacity = msg.getFieldAsInt32(NXCPCodes.VID_RATED_CAPACITY);
   }

   /**
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "CoolingZone";
   }

   /**
    * Get zone type.
    *
    * @return zone type
    */
   public CoolingZoneType getZoneType()
   {
      return zoneType;
   }

   /**
    * Get rated thermal capacity in watts.
    *
    * @return rated capacity in watts or 0 if undeclared
    */
   public int getRatedCapacity()
   {
      return ratedCapacity;
   }
}
