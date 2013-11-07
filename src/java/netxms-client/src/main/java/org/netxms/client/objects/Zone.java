/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Zone object
 */
public class Zone extends GenericObject
{
	private long zoneId;
	private long agentProxy;
	private long snmpProxy;
	private long icmpProxy;
	
	/**
	 * Create zone object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param session owning session
	 */
	public Zone(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		zoneId = msg.getVariableAsInt64(NXCPCodes.VID_ZONE_ID);
		agentProxy = msg.getVariableAsInt64(NXCPCodes.VID_AGENT_PROXY);
		snmpProxy = msg.getVariableAsInt64(NXCPCodes.VID_SNMP_PROXY);
		icmpProxy = msg.getVariableAsInt64(NXCPCodes.VID_ICMP_PROXY);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

	/**
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}

	/**
	 * @return the agentProxy
	 */
	public long getAgentProxy()
	{
		return agentProxy;
	}

	/**
	 * @return the snmpProxy
	 */
	public long getSnmpProxy()
	{
		return snmpProxy;
	}

	/**
	 * @return the icmpProxy
	 */
	public long getIcmpProxy()
	{
		return icmpProxy;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Zone";
	}
}
