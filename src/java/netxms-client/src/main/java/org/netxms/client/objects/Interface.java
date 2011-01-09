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
import org.netxms.base.*;
import org.netxms.client.MacAddress;
import org.netxms.client.NXCSession;

/**
 * Network interface object
 *
 */
public class Interface extends GenericObject
{
	private InetAddress subnetMask;
	private int ifIndex;
	private int ifType;
	private int slot;
	private int port;
	private MacAddress macAddress;
	private int requiredPollCount;
	
	/**
	 * @param msg
	 */
	public Interface(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		subnetMask = msg.getVariableAsInetAddress(NXCPCodes.VID_IP_NETMASK);
		ifIndex = msg.getVariableAsInteger(NXCPCodes.VID_IF_INDEX);
		ifType = msg.getVariableAsInteger(NXCPCodes.VID_IF_TYPE);
		slot = msg.getVariableAsInteger(NXCPCodes.VID_IF_SLOT);
		port = msg.getVariableAsInteger(NXCPCodes.VID_IF_PORT);
		macAddress = new MacAddress(msg.getVariableAsBinary(NXCPCodes.VID_MAC_ADDR));
		requiredPollCount = msg.getVariableAsInteger(NXCPCodes.VID_REQUIRED_POLLS);
	}

	/**
	 * @return Interface subnet mask
	 */
	public InetAddress getSubnetMask()
	{
		return subnetMask;
	}

	/**
	 * @return Interface index
	 */
	public int getIfIndex()
	{
		return ifIndex;
	}

	/**
	 * @return Interface type
	 */
	public int getIfType()
	{
		return ifType;
	}

	/**
	 * @return Interface MAC address
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @return Number of polls required to change interface status
	 */
	public int getRequiredPollCount()
	{
		return requiredPollCount;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Interface";
	}

	/**
	 * @return the slot
	 */
	public int getSlot()
	{
		return slot;
	}

	/**
	 * @return the port
	 */
	public int getPort()
	{
		return port;
	}
}
