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
package org.netxms.client.topology;

import java.net.InetAddress;

import org.netxms.base.NXCPMessage;

/**
 * Information about single hop in network path
 */
public class HopInfo
{
	private long nodeId;
	private InetAddress nextHop;
	private int ifIndex;
	private boolean isVpn;
	
	/**
	 * Create hop info object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	protected HopInfo(NXCPMessage msg, long baseId)
	{
		nodeId = msg.getVariableAsInt64(baseId);
		nextHop = msg.getVariableAsInetAddress(baseId + 1);
		ifIndex = msg.getVariableAsInteger(baseId + 2);
		isVpn = msg.getVariableAsBoolean(baseId + 3);
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the nextHop
	 */
	public InetAddress getNextHop()
	{
		return nextHop;
	}

	/**
	 * @return the ifIndex
	 */
	public int getIfIndex()
	{
		return ifIndex;
	}

	/**
	 * @return the isVpn
	 */
	public boolean isVpn()
	{
		return isVpn;
	}
}
