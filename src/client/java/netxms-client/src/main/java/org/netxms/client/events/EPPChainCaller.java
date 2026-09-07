/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.client.events;

import java.util.UUID;
import org.netxms.base.NXCPMessage;

/**
 * Rule calling an event processing policy chain (reported before chain deletion)
 */
public class EPPChainCaller
{
   private UUID ruleGuid;
   private int chainId;
   private int ruleNumber;
   private String ruleComments;

   /**
    * Create from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public EPPChainCaller(NXCPMessage msg, long baseId)
   {
      ruleGuid = msg.getFieldAsUUID(baseId);
      chainId = msg.getFieldAsInt32(baseId + 1);
      ruleNumber = msg.getFieldAsInt32(baseId + 2);
      ruleComments = msg.getFieldAsString(baseId + 3);
   }

   /**
    * @return GUID of the calling rule
    */
   public UUID getRuleGuid()
   {
      return ruleGuid;
   }

   /**
    * @return ID of the chain containing the calling rule (0 = main chain)
    */
   public int getChainId()
   {
      return chainId;
   }

   /**
    * @return 1-based number of the calling rule within its chain
    */
   public int getRuleNumber()
   {
      return ruleNumber;
   }

   /**
    * @return comments of the calling rule
    */
   public String getRuleComments()
   {
      return ruleComments;
   }
}
