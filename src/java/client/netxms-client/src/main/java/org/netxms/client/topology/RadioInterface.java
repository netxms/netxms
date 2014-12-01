/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.netxms.client.MacAddress;
import org.netxms.client.objects.AccessPoint;

/**
 * Radio interface information
 */
public class RadioInterface
{
	private AccessPoint accessPoint;
	private int index;
	private String name;
	private MacAddress macAddress;
	private int channel;
	private int powerDBm;
	private int powerMW;
	
	/**
	 * Create radio interface object from NXCP message
	 * 
	 * @param msg
	 * @param baseId
	 */
	public RadioInterface(AccessPoint ap, NXCPMessage msg, long baseId)
	{
		accessPoint = ap;
		index = msg.getFieldAsInt32(baseId);
		name = msg.getFieldAsString(baseId + 1);
		macAddress = new MacAddress(msg.getFieldAsBinary(baseId + 2));
		channel = msg.getFieldAsInt32(baseId + 3);
		powerDBm = msg.getFieldAsInt32(baseId + 4);
		powerMW = msg.getFieldAsInt32(baseId + 5);
	}

	/**
	 * @return the index
	 */
	public int getIndex()
	{
		return index;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the macAddress
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @return the channel
	 */
	public int getChannel()
	{
		return channel;
	}

	/**
	 * Get transmitting power in dBm
	 * 
	 * @return the powerDBm
	 */
	public int getPowerDBm()
	{
		return powerDBm;
	}

	/**
	 * Get transmitting power in milliwatts
	 * 
	 * @return the powerMW
	 */
	public int getPowerMW()
	{
		return powerMW;
	}

	/**
	 * @return the accessPoint
	 */
	public AccessPoint getAccessPoint()
	{
		return accessPoint;
	}
}
