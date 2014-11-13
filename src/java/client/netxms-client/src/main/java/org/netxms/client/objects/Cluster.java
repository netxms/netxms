/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * @author victor
 *
 */
public class Cluster extends GenericObject
{
	private int clusterType;
	private List<ClusterSyncNetwork> syncNetworks = new ArrayList<ClusterSyncNetwork>(1);
	private List<ClusterResource> resources = new ArrayList<ClusterResource>();
	private long zoneId;
	
	/**
	 * @param msg
	 * @param session
	 */
	public Cluster(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		clusterType = msg.getVariableAsInteger(NXCPCodes.VID_CLUSTER_TYPE);
		zoneId = msg.getVariableAsInt64(NXCPCodes.VID_ZONE_ID);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_SYNC_SUBNETS);
		if (count > 0)
		{
			long[] sn = msg.getVariableAsUInt32Array(NXCPCodes.VID_SYNC_SUBNETS);
			for(int i = 0; i < sn.length;)
			{
				InetAddress addr = inetAddressFromInt32(sn[i++]);
				InetAddress mask = inetAddressFromInt32(sn[i++]);
				syncNetworks.add(new ClusterSyncNetwork(addr, mask));
			}
		}
		
		count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_RESOURCES);
		long baseId = NXCPCodes.VID_RESOURCE_LIST_BASE;
		for(int i = 0; i < count; i++, baseId += 10)
		{
			resources.add(new ClusterResource(msg, baseId));
		}
	}
	
	/**
	 * Create InetAddress object from 32bit integer
	 * 
	 * @param intVal
	 * @return
	 */
	private InetAddress inetAddressFromInt32(long intVal)
	{
		final byte[] addr = new byte[4];
		InetAddress inetAddr;
		
		addr[0] =  (byte)((intVal & 0xFF000000) >> 24);
		addr[1] =  (byte)((intVal & 0x00FF0000) >> 16);
		addr[2] =  (byte)((intVal & 0x0000FF00) >> 8);
		addr[3] =  (byte)(intVal & 0x000000FF);
		
		try
		{
			inetAddr = InetAddress.getByAddress(addr);
		}
		catch(UnknownHostException e)
		{
			inetAddr = null;
		}
		return inetAddr;
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
	public List<ClusterSyncNetwork> getSyncNetworks()
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

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Cluster";
	}

	/**
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}
}
