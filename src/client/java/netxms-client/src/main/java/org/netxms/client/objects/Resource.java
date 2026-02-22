/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.util.Date;
import java.util.Map;
import java.util.Set;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.PollState;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.objects.interfaces.PollingTarget;

/**
 * Cloud resource object
 */
public class Resource extends DataCollectionTarget implements PollingTarget
{
   private String cloudResourceId;
   private String connectorName;
   private String resourceType;
   private String region;
   private int resourceState;
   private String providerState;
   private long linkedNodeId;
   private String accountId;
   private Date lastDiscoveryTime;
   private String connectorData;
   private Map<String, String> tags;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public Resource(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      cloudResourceId = msg.getFieldAsString(NXCPCodes.VID_CLOUD_RESOURCE_ID);
      connectorName = msg.getFieldAsString(NXCPCodes.VID_CONNECTOR_NAME);
      resourceType = msg.getFieldAsString(NXCPCodes.VID_RESOURCE_TYPE);
      region = msg.getFieldAsString(NXCPCodes.VID_CLOUD_REGION);
      resourceState = msg.getFieldAsInt16(NXCPCodes.VID_RESOURCE_STATE);
      providerState = msg.getFieldAsString(NXCPCodes.VID_PROVIDER_STATE);
      linkedNodeId = msg.getFieldAsInt32(NXCPCodes.VID_LINKED_NODE_ID);
      accountId = msg.getFieldAsString(NXCPCodes.VID_ACCOUNT_ID);
      lastDiscoveryTime = msg.getFieldAsDate(NXCPCodes.VID_LAST_DISCOVERY_TIME);
      connectorData = msg.getFieldAsString(NXCPCodes.VID_CONNECTOR_DATA);
      tags = msg.getStringMapFromFields(NXCPCodes.VID_RESOURCE_TAG_LIST_BASE, NXCPCodes.VID_NUM_TAGS);
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
    * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "Resource";
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, cloudResourceId);
      addString(strings, connectorName);
      addString(strings, resourceType);
      addString(strings, region);
      addString(strings, providerState);
      addString(strings, accountId);
      addString(strings, connectorData);
      return strings;
   }

   /**
    * @return the cloudResourceId
    */
   public String getCloudResourceId()
   {
      return cloudResourceId;
   }

   /**
    * @return the connectorName
    */
   public String getConnectorName()
   {
      return connectorName;
   }

   /**
    * @return the resourceType
    */
   public String getResourceType()
   {
      return resourceType;
   }

   /**
    * @return the region
    */
   public String getRegion()
   {
      return region;
   }

   /**
    * @return the resourceState
    */
   public int getResourceState()
   {
      return resourceState;
   }

   /**
    * @return the providerState
    */
   public String getProviderState()
   {
      return providerState;
   }

   /**
    * @return the linkedNodeId
    */
   public long getLinkedNodeId()
   {
      return linkedNodeId;
   }

   /**
    * @return the accountId
    */
   public String getAccountId()
   {
      return accountId;
   }

   /**
    * @return the lastDiscoveryTime
    */
   public Date getLastDiscoveryTime()
   {
      return lastDiscoveryTime;
   }

   /**
    * @return the connectorData
    */
   public String getConnectorData()
   {
      return connectorData;
   }

   /**
    * @return the tags
    */
   public Map<String, String> getTags()
   {
      return tags;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getFlags()
    */
   @Override
   public int getFlags()
   {
      return flags;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getIfXTablePolicy()
    */
   @Override
   public int getIfXTablePolicy()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getAgentCacheMode()
    */
   @Override
   public AgentCacheMode getAgentCacheMode()
   {
      return null;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getPollerNodeId()
    */
   @Override
   public long getPollerNodeId()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveAgent()
    */
   @Override
   public boolean canHaveAgent()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveInterfaces()
    */
   @Override
   public boolean canHaveInterfaces()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHavePollerNode()
    */
   @Override
   public boolean canHavePollerNode()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canUseEtherNetIP()
    */
   @Override
   public boolean canUseEtherNetIP()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canUseModbus()
    */
   @Override
   public boolean canUseModbus()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getPollStates()
    */
   @Override
   public PollState[] getPollStates()
   {
      return pollStates;
   }
}
