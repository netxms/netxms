/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
import org.netxms.client.constants.BusinessServiceState;

/**
 * Business service representation
 */
public class BusinessService extends BaseBusinessService
{
   private BusinessServiceState serviceState;
   private String instance;

   /**
    * Constructor
    * 
    * @param msg NXCPMessage with data
    * @param session NXCPSession 
    */
   public BusinessService(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      serviceState = BusinessServiceState.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_SERVICE_STATUS));
      instance = msg.getFieldAsString(NXCPCodes.VID_INSTANCE);
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
    */
   @Override
   public boolean isAllowedOnMap()
   {
      return true;
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
    * Get business service state.
    *
    * @return business service state
    */
   public BusinessServiceState getServiceState()
   {
      return serviceState;
   }

   /**
    * Get instance name.
    *
    * @return instance name
    */
   public String getInstance()
   {
      return instance;
   }

   /**
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "BusinessService";
   }
}
