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
import org.netxms.client.constants.PowerDomainType;

/**
 * Power domain object - electrical distribution node within a facility
 */
public class PowerDomain extends DataCollectionContainer
{
   private PowerDomainType domainType;
   private String feedTag;
   private int ratedPower;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public PowerDomain(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      domainType = PowerDomainType.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_DOMAIN_TYPE));
      feedTag = msg.getFieldAsString(NXCPCodes.VID_FEED_TAG);
      if (feedTag == null)
         feedTag = "";
      ratedPower = msg.getFieldAsInt32(NXCPCodes.VID_RATED_POWER);
   }

   /**
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "PowerDomain";
   }

   /**
    * Get domain type.
    *
    * @return domain type
    */
   public PowerDomainType getDomainType()
   {
      return domainType;
   }

   /**
    * Get feed tag (free label, conventionally "A" or "B").
    *
    * @return feed tag or empty string
    */
   public String getFeedTag()
   {
      return feedTag;
   }

   /**
    * Get rated (nameplate) power in watts.
    *
    * @return rated power in watts or 0 if undeclared
    */
   public int getRatedPower()
   {
      return ratedPower;
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, feedTag);
      return strings;
   }
}
