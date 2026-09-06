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

import java.util.Set;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Facility object - reporting boundary for data center energy monitoring
 */
public class Facility extends DataCollectionContainer
{
   private int settlementLag;
   private String providerId;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public Facility(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      settlementLag = msg.getFieldAsInt32(NXCPCodes.VID_SETTLEMENT_LAG);
      providerId = msg.getFieldAsString(NXCPCodes.VID_PROVIDER_ID);
      if (providerId == null)
         providerId = "";
   }

   /**
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "Facility";
   }

   /**
    * Get settlement lag (number of days a provisional day waits before settlement).
    *
    * @return settlement lag in days
    */
   public int getSettlementLag()
   {
      return settlementLag;
   }

   /**
    * Get selected KPI computation provider ID. Empty string means that engine is disabled for this facility.
    *
    * @return computation provider ID or empty string
    */
   public String getProviderId()
   {
      return providerId;
   }

   /**
    * Check if KPI computation engine is enabled for this facility (computation provider is selected).
    *
    * @return true if engine is enabled
    */
   public boolean isEngineEnabled()
   {
      return !providerId.isEmpty();
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, providerId);
      return strings;
   }
}
