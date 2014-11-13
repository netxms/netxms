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

import org.netxms.base.NXCPMessage;

/**
 * Contains information about VLAN
 */
public class VlanInfo
{
	private int vlanId;
	private String name;
	private Port[] ports;
	
	/**
	 * Create VLAN information object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable id
	 */
	public VlanInfo(NXCPMessage msg, long baseId)
	{
		vlanId = msg.getVariableAsInteger(baseId);
		name = msg.getVariableAsString(baseId + 1);
		
		int count = msg.getVariableAsInteger(baseId + 2);
		ports = new Port[count];
		long[] sps = msg.getVariableAsUInt32Array(baseId + 3);
		long[] indexes = msg.getVariableAsUInt32Array(baseId + 4);
		long[] ids = msg.getVariableAsUInt32Array(baseId + 5);
		for(int i = 0; i < count; i++)
			ports[i] = new Port(ids[i], indexes[i], (int)(sps[i] >> 16), (int)(sps[i] & 0xFFFF));
	}

	/**
	 * @return the vlanId
	 */
	public int getVlanId()
	{
		return vlanId;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the ports
	 */
	public Port[] getPorts()
	{
		return ports;
	}
}
