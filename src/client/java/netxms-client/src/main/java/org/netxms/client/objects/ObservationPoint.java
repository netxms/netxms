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

/**
 * Observation point object - represents one monitored interface / observation point of a traffic observer
 */
public class ObservationPoint extends DataCollectionTarget implements PollingTarget
{
   // Observation point states
   public static final int STATE_UNKNOWN = 0;
   public static final int STATE_ACTIVE = 1;
   public static final int STATE_INACTIVE = 2;

   private long observerId;
   private String externalId;
   private String pointType;
   private boolean inScope;
   private int zoneUIN;
   private int samplingRate;
   private String localNetworks;
   private int state;
   private String providerState;
   private Date lastDiscoveryTime;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public ObservationPoint(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      observerId = msg.getFieldAsInt64(NXCPCodes.VID_TRAFFIC_OBSERVER_ID);
      externalId = msg.getFieldAsString(NXCPCodes.VID_CLOUD_RESOURCE_ID);
      pointType = msg.getFieldAsString(NXCPCodes.VID_RESOURCE_TYPE);
      inScope = msg.getFieldAsBoolean(NXCPCodes.VID_IN_SCOPE);
      zoneUIN = msg.getFieldAsInt32(NXCPCodes.VID_ZONE_UIN);
      samplingRate = msg.getFieldAsInt32(NXCPCodes.VID_SAMPLING_RATE);
      localNetworks = msg.getFieldAsString(NXCPCodes.VID_LOCAL_NETWORKS);
      state = msg.getFieldAsInt16(NXCPCodes.VID_RESOURCE_STATE);
      providerState = msg.getFieldAsString(NXCPCodes.VID_PROVIDER_STATE);
      lastDiscoveryTime = msg.getFieldAsDate(NXCPCodes.VID_LAST_DISCOVERY_TIME);
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
      return "ObservationPoint";
   }

   /**
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, externalId);
      addString(strings, pointType);
      return strings;
   }

   /**
    * @return object ID of owning traffic observer
    */
   public long getObserverId()
   {
      return observerId;
   }

   /**
    * @return connector-side point identity
    */
   public String getExternalId()
   {
      return externalId;
   }

   /**
    * @return connector-reported point type (packet / zmq / pcap / ...)
    */
   public String getPointType()
   {
      return pointType;
   }

   /**
    * @return true if this point is in scope for host matching and host data collection
    */
   public boolean isInScope()
   {
      return inScope;
   }

   /**
    * @return zone UIN for host matching (-1 = inherit from observer)
    */
   public int getZoneUIN()
   {
      return zoneUIN;
   }

   /**
    * @return sampling rate at this observation point (0 = unknown, 1 = unsampled, N = 1:N sampling)
    */
   public int getSamplingRate()
   {
      return samplingRate;
   }

   /**
    * @return local networks as reported by the analyzer (comma-separated CIDR list)
    */
   public String getLocalNetworks()
   {
      return localNetworks;
   }

   /**
    * @return the state
    */
   public int getState()
   {
      return state;
   }

   /**
    * @return raw analyzer-side state
    */
   public String getProviderState()
   {
      return providerState;
   }

   /**
    * @return the lastDiscoveryTime
    */
   public Date getLastDiscoveryTime()
   {
      return lastDiscoveryTime;
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
