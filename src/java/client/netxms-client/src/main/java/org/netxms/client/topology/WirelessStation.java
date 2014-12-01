/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.netxms.client.MacAddress;

/**
 * Information about wireless station
 */
public class WirelessStation
{
	private MacAddress macAddress;
	private InetAddress ipAddress;
	private long nodeObjectId;
	private long accessPointId;
	private String radioInterface;
	private String ssid;
	private int vlan;
	
	/**
	 * @param msg
	 * @param baseId
	 */
	public WirelessStation(NXCPMessage msg, long baseId)
	{
		macAddress = new MacAddress(msg.getFieldAsBinary(baseId));
		ipAddress = msg.getFieldAsInetAddress(baseId + 1);
		ssid = msg.getFieldAsString(baseId + 2);
		vlan = msg.getFieldAsInt32(baseId + 3);
		accessPointId = msg.getFieldAsInt64(baseId + 4);
		radioInterface = msg.getFieldAsString(baseId + 6);
		nodeObjectId = msg.getFieldAsInt64(baseId + 7);
	}

	/**
	 * @return the macAddress
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @return the ipAddress
	 */
	public InetAddress getIpAddress()
	{
		return ipAddress;
	}

	/**
	 * @return the nodeObjectId
	 */
	public long getNodeObjectId()
	{
		return nodeObjectId;
	}

	/**
	 * @return the accessPointId
	 */
	public long getAccessPointId()
	{
		return accessPointId;
	}

	/**
	 * @return the radioInterface
	 */
	public String getRadioInterface()
	{
		return radioInterface;
	}

	/**
	 * @return the ssid
	 */
	public String getSsid()
	{
		return ssid;
	}

	/**
	 * @return the vlan
	 */
	public int getVlan()
	{
		return vlan;
	}
}
