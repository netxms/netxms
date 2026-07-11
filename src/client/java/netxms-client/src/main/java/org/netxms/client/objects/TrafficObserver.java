/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import org.netxms.client.objects.interfaces.ZoneMember;

/**
 * Traffic observer object - represents one external traffic analysis instance
 */
public class TrafficObserver extends DataCollectionTarget implements PollingTarget, ZoneMember
{
   // Connection states
   public static final int STATE_UNKNOWN = 0;
   public static final int STATE_CONNECTED = 1;
   public static final int STATE_UNREACHABLE = 2;
   public static final int STATE_AUTH_FAILURE = 3;

   // Capabilities (bit mask)
   public static final long CAP_HOST_L7 = 0x00000001L;
   public static final long CAP_HOST_PEERS = 0x00000002L;
   public static final long CAP_POINT_L7 = 0x00000004L;
   public static final long CAP_POINT_TOP_TALKERS = 0x00000008L;
   public static final long CAP_POINT_DSCP = 0x00000010L;
   public static final long CAP_HISTORICAL_TIMESERIES = 0x00000020L;
   public static final long CAP_SYNC_HOST_ALIASES = 0x00000040L;
   public static final long CAP_HOST_SET_AUTHORITATIVE = 0x00000080L;

   private String connectorName;
   private boolean hasCredentials;
   private int zoneUIN;
   private long linkedNodeId;
   private int removalPolicy;
   private int gracePeriod;
   private String syncConfig;
   private int lastDiscoveryStatus;
   private Date lastDiscoveryTime;
   private String lastDiscoveryMessage;
   private int connectionState;
   private String backendProduct;
   private String backendVersion;
   private String backendEdition;
   private long capabilities;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public TrafficObserver(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      connectorName = msg.getFieldAsString(NXCPCodes.VID_CONNECTOR_NAME);
      hasCredentials = msg.getFieldAsBoolean(NXCPCodes.VID_CLOUD_CREDENTIALS);
      zoneUIN = msg.getFieldAsInt32(NXCPCodes.VID_ZONE_UIN);
      linkedNodeId = msg.getFieldAsInt64(NXCPCodes.VID_LINKED_NODE_ID);
      removalPolicy = msg.getFieldAsInt16(NXCPCodes.VID_REMOVAL_POLICY);
      gracePeriod = msg.getFieldAsInt32(NXCPCodes.VID_GRACE_PERIOD);
      syncConfig = msg.getFieldAsString(NXCPCodes.VID_SYNC_CONFIG);
      lastDiscoveryStatus = msg.getFieldAsInt16(NXCPCodes.VID_LAST_DISCOVERY_STATUS);
      lastDiscoveryTime = msg.getFieldAsDate(NXCPCodes.VID_LAST_DISCOVERY_TIME);
      lastDiscoveryMessage = msg.getFieldAsString(NXCPCodes.VID_LAST_DISCOVERY_MESSAGE);
      connectionState = msg.getFieldAsInt16(NXCPCodes.VID_CONNECTION_STATE);
      backendProduct = msg.getFieldAsString(NXCPCodes.VID_BACKEND_PRODUCT);
      backendVersion = msg.getFieldAsString(NXCPCodes.VID_VERSION);
      backendEdition = msg.getFieldAsString(NXCPCodes.VID_BACKEND_EDITION);
      capabilities = msg.getFieldAsInt64(NXCPCodes.VID_CAPABILITIES);
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
      return "TrafficObserver";
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, connectorName);
      addString(strings, backendProduct);
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
    * @return true if connector credentials are set
    */
   public boolean hasCredentials()
   {
      return hasCredentials;
   }

   /**
    * @see org.netxms.client.objects.interfaces.ZoneMember#getZoneId()
    */
   @Override
   public int getZoneId()
   {
      return zoneUIN;
   }

   /**
    * @see org.netxms.client.objects.interfaces.ZoneMember#getZoneName()
    */
   @Override
   public String getZoneName()
   {
      Zone zone = session.findZone(zoneUIN);
      return (zone != null) ? zone.getObjectName() : Long.toString(zoneUIN);
   }

   /**
    * @return ID of the node representing the analyzer itself (0 = not linked)
    */
   public long getLinkedNodeId()
   {
      return linkedNodeId;
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
    * @return the syncConfig
    */
   public String getSyncConfig()
   {
      return syncConfig;
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
    * @return the connectionState
    */
   public int getConnectionState()
   {
      return connectionState;
   }

   /**
    * @return the backendProduct
    */
   public String getBackendProduct()
   {
      return backendProduct;
   }

   /**
    * @return the backendVersion
    */
   public String getBackendVersion()
   {
      return backendVersion;
   }

   /**
    * @return the backendEdition
    */
   public String getBackendEdition()
   {
      return backendEdition;
   }

   /**
    * @return the capabilities
    */
   public long getCapabilities()
   {
      return capabilities;
   }

   /**
    * Check if backend has given capability.
    *
    * @param capability capability bit to check
    * @return true if backend has given capability
    */
   public boolean hasCapability(long capability)
   {
      return (capabilities & capability) != 0;
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
