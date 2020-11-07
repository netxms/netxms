/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.InetAddressEx;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.AgentCacheMode;

/**
 * Cluster object
 */
public class Cluster extends DataCollectionTarget implements ZoneMember, PollingTarget
{
	private int clusterType;
	private List<InetAddressEx> syncNetworks = new ArrayList<InetAddressEx>(1);
	private List<ClusterResource> resources = new ArrayList<ClusterResource>();
	private int zoneId;
	
	/**
	 * @param msg
	 * @param session
	 */
	public Cluster(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		clusterType = msg.getFieldAsInt32(NXCPCodes.VID_CLUSTER_TYPE);
		zoneId = msg.getFieldAsInt32(NXCPCodes.VID_ZONE_UIN);
		
		int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_SYNC_SUBNETS);
      long fieldId = NXCPCodes.VID_SYNC_SUBNETS_BASE;
		for(int i = 0; i < count; i++)
		{
		   syncNetworks.add(msg.getFieldAsInetAddressEx(fieldId++));
		}
		
		count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_RESOURCES);
		fieldId = NXCPCodes.VID_RESOURCE_LIST_BASE;
		for(int i = 0; i < count; i++, fieldId += 10)
		{
			resources.add(new ClusterResource(msg, fieldId));
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

	/* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#isAlarmsVisible()
    */
   @Override
   public boolean isAlarmsVisible()
   {
      return true;
   }

   /**
	 * @return the clusterType
	 */
	public int getClusterType()
	{
		return clusterType;
	}

	/**
	 * @return the syncNetworks
	 */
	public List<InetAddressEx> getSyncNetworks()
	{
		return syncNetworks;
	}

	/**
	 * @return the resources
	 */
	public List<ClusterResource> getResources()
	{
		return resources;
	}

	@Override
	public String getObjectClassName()
	{
		return "Cluster";
	}

   /**
    * @see org.netxms.client.objects.ZoneMember#getZoneId()
    */
   @Override
   public int getZoneId()
   {
      return zoneId;
   }

   /**
    * @see org.netxms.client.objects.ZoneMember#getZoneName()
    */
   @Override
   public String getZoneName()
   {
      Zone zone = session.findZone(zoneId);
      return (zone != null) ? zone.getObjectName() : Long.toString(zoneId);
   }

   /**
    * @see org.netxms.client.objects.PollingTarget#getIfXTablePolicy()
    */
   @Override
   public int getIfXTablePolicy()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.PollingTarget#getAgentCacheMode()
    */
   @Override
   public AgentCacheMode getAgentCacheMode()
   {
      return null;
   }

   /**
    * @see org.netxms.client.objects.PollingTarget#getFlags()
    */
   @Override
   public int getFlags()
   {
      return flags;
   }

   /**
    * @see org.netxms.client.objects.PollingTarget#getPollerNodeId()
    */
   @Override
   public long getPollerNodeId()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.PollingTarget#canHaveAgent()
    */
   @Override
   public boolean canHaveAgent()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.PollingTarget#canHaveInterfaces()
    */
   @Override
   public boolean canHaveInterfaces()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.PollingTarget#canHavePollerNode()
    */
   @Override
   public boolean canHavePollerNode()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.PollingTarget#canUseEtherNetIP()
    */
   @Override
   public boolean canUseEtherNetIP()
   {
      return false;
   }
}
