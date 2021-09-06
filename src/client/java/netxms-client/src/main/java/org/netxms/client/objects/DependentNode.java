/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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

import org.netxms.base.NXCPMessage;

/**
 * Dependent node information
 */
public final class DependentNode
{
   public static final int AGENT_PROXY = 0x01;
   public static final int SNMP_PROXY = 0x02;
   public static final int ICMP_PROXY = 0x04;
   public static final int DATA_COLLECTION_SOURCE = 0x08;
   
   private long nodeId;
   private int dependencyTypes;

   /**
    * Create from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public DependentNode(final NXCPMessage msg, long baseId)
   {
      nodeId = msg.getFieldAsInt64(baseId);
      dependencyTypes = msg.getFieldAsInt32(baseId + 1);
   }

   /**
    * Get ID of dependent node
    * 
    * @return ID of dependent node
    */
   public long getNodeId()
   {
      return nodeId;
   }
   
   /**
    * Returns true if original node is an agent proxy for this node
    * 
    * @return true if original node is an agent proxy for this node
    */
   public boolean isAgentProxy()
   {
      return (dependencyTypes & AGENT_PROXY) != 0;
   }
   
   /**
    * Returns true if original node is an SNMP proxy for this node
    * 
    * @return true if original node is an SNMP proxy for this node
    */
   public boolean isSnmpProxy()
   {
      return (dependencyTypes & SNMP_PROXY) != 0;
   }
   
   /**
    * Returns true if original node is an ICMP proxy for this node
    * 
    * @return true if original node is an ICMP proxy for this node
    */
   public boolean isIcmpProxy()
   {
      return (dependencyTypes & ICMP_PROXY) != 0;
   }
   
   /**
    * Returns true if original node is a data collection source for this node
    * 
    * @return true if original node is a data collection source for this node
    */
   public boolean isDataCollectionSource()
   {
      return (dependencyTypes & DATA_COLLECTION_SOURCE) != 0;
   }
}
