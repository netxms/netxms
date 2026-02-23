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
import java.util.Set;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.PollState;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.objects.interfaces.PollingTarget;

/**
 * Cloud domain object
 */
public class CloudDomain extends DataCollectionTarget implements PollingTarget
{
   private String connectorName;
   private boolean hasCredentials;
   private String discoverySchedule;
   private String discoveryFilter;
   private int removalPolicy;
   private int gracePeriod;
   private int defaultPollingInterval;
   private boolean autoDiscoverChildren;
   private boolean autoProvisionDCI;
   private int lastDiscoveryStatus;
   private Date lastDiscoveryTime;
   private String lastDiscoveryMessage;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public CloudDomain(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      connectorName = msg.getFieldAsString(NXCPCodes.VID_CONNECTOR_NAME);
      hasCredentials = msg.getFieldAsBoolean(NXCPCodes.VID_CLOUD_CREDENTIALS);
      discoverySchedule = msg.getFieldAsString(NXCPCodes.VID_DISCOVERY_SCHEDULE);
      discoveryFilter = msg.getFieldAsString(NXCPCodes.VID_DISCOVERY_FILTER);
      removalPolicy = msg.getFieldAsInt16(NXCPCodes.VID_REMOVAL_POLICY);
      gracePeriod = msg.getFieldAsInt32(NXCPCodes.VID_GRACE_PERIOD);
      defaultPollingInterval = msg.getFieldAsInt32(NXCPCodes.VID_DEFAULT_POLL_INTERVAL);
      autoDiscoverChildren = msg.getFieldAsBoolean(NXCPCodes.VID_AUTO_DISCOVER_CHILDREN);
      autoProvisionDCI = msg.getFieldAsBoolean(NXCPCodes.VID_AUTO_PROVISION_DCI);
      lastDiscoveryStatus = msg.getFieldAsInt16(NXCPCodes.VID_LAST_DISCOVERY_STATUS);
      lastDiscoveryTime = msg.getFieldAsDate(NXCPCodes.VID_LAST_DISCOVERY_TIME);
      lastDiscoveryMessage = msg.getFieldAsString(NXCPCodes.VID_LAST_DISCOVERY_MESSAGE);
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
      return "CloudDomain";
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, connectorName);
      addString(strings, discoverySchedule);
      addString(strings, lastDiscoveryMessage);
      return strings;
   }

   /**
    * @return the connectorName
    */
   public String getConnectorName()
   {
      return connectorName;
   }

   /**
    * @return true if cloud credentials are set
    */
   public boolean hasCredentials()
   {
      return hasCredentials;
   }

   /**
    * @return the discoverySchedule
    */
   public String getDiscoverySchedule()
   {
      return discoverySchedule;
   }

   /**
    * @return the discoveryFilter
    */
   public String getDiscoveryFilter()
   {
      return discoveryFilter;
   }

   /**
    * @return the removalPolicy
    */
   public int getRemovalPolicy()
   {
      return removalPolicy;
   }

   /**
    * @return the gracePeriod
    */
   public int getGracePeriod()
   {
      return gracePeriod;
   }

   /**
    * @return the defaultPollingInterval
    */
   public int getDefaultPollingInterval()
   {
      return defaultPollingInterval;
   }

   /**
    * @return true if auto-discovery of children is enabled
    */
   public boolean isAutoDiscoverChildren()
   {
      return autoDiscoverChildren;
   }

   /**
    * @return true if auto-provisioning of DCIs is enabled
    */
   public boolean isAutoProvisionDCI()
   {
      return autoProvisionDCI;
   }

   /**
    * @return the lastDiscoveryStatus
    */
   public int getLastDiscoveryStatus()
   {
      return lastDiscoveryStatus;
   }

   /**
    * @return the lastDiscoveryTime
    */
   public Date getLastDiscoveryTime()
   {
      return lastDiscoveryTime;
   }

   /**
    * @return the lastDiscoveryMessage
    */
   public String getLastDiscoveryMessage()
   {
      return lastDiscoveryMessage;
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
