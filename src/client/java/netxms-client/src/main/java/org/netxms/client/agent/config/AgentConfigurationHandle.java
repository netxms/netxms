/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.client.agent.config;

import org.netxms.base.NXCPMessage;

/**
 * Handle for server side agent configuration (contains only metadata without actual content).
 */
public class AgentConfigurationHandle implements Comparable<AgentConfigurationHandle>
{
   private long id;
   private String name;
   private long sequenceNumber;

   /**
    * Create empty handle
    */
   public AgentConfigurationHandle()
   {
      id = 0;
      name = "New configuration";
      sequenceNumber = -1;
   }

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId ID of first field
    */
   public AgentConfigurationHandle(NXCPMessage msg, long baseId)
   {
      this.id = msg.getFieldAsInt64(baseId);
      this.name = msg.getFieldAsString(baseId + 1);
      this.sequenceNumber = msg.getFieldAsInt64(baseId + 2);
   }

   /**
    * Get configuration ID.
    * 
    * @return configuration ID
    */
   public long getId()
   {
      return id;
   }

   /**
    * Get configuration name.
    * 
    * @return configuration name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get configuration sequence number (priority).
    *
    * @return configuration sequence number
    */
   public long getSequenceNumber()
   {
      return sequenceNumber;
   }

   /**
    * @see java.lang.Comparable#compareTo(java.lang.Object)
    */
   @Override
   public int compareTo(AgentConfigurationHandle e)
   {
      return Long.signum(sequenceNumber - e.sequenceNumber);
   }
}
