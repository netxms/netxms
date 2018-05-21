/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.client;

import java.net.InetAddress;
import java.util.UUID;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;

/**
 * Agent tunnel information
 */
public class AgentTunnel
{
   private int id;
   private UUID guid;
   private InetAddress address;
   private long nodeId;
   private UUID agentId;
   private String systemName;
   private String systemInformation;
   private String platformName;
   private String agentVersion;
   private long zoneUIN;
   private int activeChannelCount;
   private String hostname;
   
   /**
    * Create from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected AgentTunnel(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      guid = msg.getFieldAsUUID(baseId + 1);
      nodeId = msg.getFieldAsInt64(baseId + 2);
      address = msg.getFieldAsInetAddress(baseId + 3);
      systemName = msg.getFieldAsString(baseId + 4);
      systemInformation = msg.getFieldAsString(baseId + 5);
      platformName = msg.getFieldAsString(baseId + 6);
      agentVersion = msg.getFieldAsString(baseId + 7);
      activeChannelCount = msg.getFieldAsInt32(baseId + 8);
      zoneUIN = msg.getFieldAsInt64(baseId + 9);
      hostname = msg.getFieldAsString(baseId + 10);
      agentId = msg.getFieldAsUUID(baseId + 11);
   }
   
   /**
    * Check if tunnel is bound
    * 
    * @return true if tunnel is bound
    */
   public boolean isBound()
   {
      return nodeId != 0;
   }

   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }

   /**
    * @return the guid
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @return the address
    */
   public InetAddress getAddress()
   {
      return address;
   }

   /**
    * @return the nodeId
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @return the agentId
    */
   public UUID getAgentId()
   {
      return (agentId != null) ? agentId : NXCommon.EMPTY_GUID;
   }

   /**
    * @return the systemName
    */
   public String getSystemName()
   {
      return systemName;
   }

   /**
    * @return the systemInformation
    */
   public String getSystemInformation()
   {
      return systemInformation;
   }

   /**
    * @return the platformName
    */
   public String getPlatformName()
   {
      return platformName;
   }

   /**
    * @return the agentVersion
    */
   public String getAgentVersion()
   {
      return agentVersion;
   }

   /**
    * @return the zoneId
    */
   public long getZoneUIN()
   {
      return zoneUIN;
   }

   /**
    * @return the activeChannelCount
    */
   public int getActiveChannelCount()
   {
      return activeChannelCount;
   }
   
   /**
    * @return the hostname
    */
   public String getHostname()
   {
      return hostname;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "AgentTunnel [id=" + id + ", guid=" + guid + ", address=" + address + ", nodeId=" + nodeId + ", systemName="
            + systemName + ", hostname=" + hostname + ", systemInformation=" + systemInformation + ", platformName="
            + platformName + ", agentVersion=" + agentVersion + ", zoneUIN=" + zoneUIN + ", activeChannelCount=" + activeChannelCount + "]";
   }
}
