/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import org.netxms.base.MacAddress;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.ConnectionPointType;

/**
 * Connection point information
 */
public class ConnectionPoint
{
	private long localNodeId;
	private long localInterfaceId;
	private MacAddress localMacAddress;
	private InetAddress localIpAddress;
	private long nodeId;
	private long interfaceId;
	private int interfaceIndex;
	private ConnectionPointType type; 
	private Object data;
	private boolean historical = false;

	/**
	 * Create connection point information from NXCP message
	 * 
	 * @param msg NXCP message
	 */
	public ConnectionPoint(NXCPMessage msg)
	{
		nodeId = msg.getFieldAsInt64(NXCPCodes.VID_OBJECT_ID);
		interfaceId = msg.getFieldAsInt64(NXCPCodes.VID_INTERFACE_ID);
		interfaceIndex = msg.getFieldAsInt32(NXCPCodes.VID_IF_INDEX);
		localNodeId = msg.getFieldAsInt64(NXCPCodes.VID_LOCAL_NODE_ID);
		localInterfaceId = msg.getFieldAsInt64(NXCPCodes.VID_LOCAL_INTERFACE_ID);
		localMacAddress = new MacAddress(msg.getFieldAsBinary(NXCPCodes.VID_MAC_ADDR));
		localIpAddress = msg.getFieldAsInetAddress(NXCPCodes.VID_IP_ADDRESS);
		type = ConnectionPointType.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_CONNECTION_TYPE));
	}

	/**
	 * Create connection point information from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param base NXCP message field base for lists
	 */
	public ConnectionPoint(NXCPMessage msg, long base)
	{
		localMacAddress = new MacAddress(msg.getFieldAsBinary(base));
		type = ConnectionPointType.getByValue(msg.getFieldAsInt32(base + 1));
		nodeId = msg.getFieldAsInt64(base + 2);
		interfaceId = msg.getFieldAsInt64(base + 3);
		interfaceIndex = msg.getFieldAsInt32(base + 4);
		localNodeId = msg.getFieldAsInt64(base + 5);
		localInterfaceId = msg.getFieldAsInt64(base + 6);
		localIpAddress = msg.getFieldAsInetAddress(base + 7);

	}
	
   /**
    * Create unconnected connection point information
    * 
    * @param localNodeId Local node id
    * @param localInterfaceId Local interface id
    */
   public ConnectionPoint(long localNodeId, long localInterfaceId)
   {
      this.localNodeId = localNodeId;
      this.localInterfaceId = localInterfaceId;
      type = ConnectionPointType.NOT_FOUND;
   }

	/**
	 * Create not found connection point
	 * 
	 * @param macAddress MAC address search was done for
	 */
	public ConnectionPoint(MacAddress macAddress)
	{
	   type = ConnectionPointType.NOT_FOUND;
	   localMacAddress = macAddress;
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}
	
	/** Set node ID
	 * 
	 * @param nodeId node id
	 */
	public void setNodeId(long nodeId)
	{
	   this.nodeId = nodeId;
	}

	/**
	 * @return the interfaceId
	 */
	public long getInterfaceId()
	{
		return interfaceId;
	}
	
	/** Set interface ID
	 * 
    * @param interfaceId interface id
    */
	public void setInterfaceId(long interfaceId)
	{
	   this.interfaceId = interfaceId;
	}

	/**
	 * @return the interfaceIndex
	 */
	public int getInterfaceIndex()
	{
		return interfaceIndex;
	}
	
	/** Set interface insex
    * @param interfaceIndex interface index
    */
	public void setInterfaceIndex(int interfaceIndex)
	{
	   this.interfaceIndex = interfaceIndex;
	}

	/**
	 * @return the localNodeId
	 */
	public long getLocalNodeId()
	{
		return localNodeId;
	}

	/**
	 * @return the localInterfaceId
	 */
	public long getLocalInterfaceId()
	{
		return localInterfaceId;
	}

	/**
	 * @return the localMacAddress
	 */
	public MacAddress getLocalMacAddress()
	{
		return localMacAddress;
	}

	/**
	 * Get user data.
	 * 
	 * @return user data
	 */
	public Object getData()
	{
		return data;
	}

	/**
	 * Set user data.
	 * 
	 * @param data user data
	 */
	public void setData(Object data)
	{
		this.data = data;
	}

	/**
	 * @return the localIpAddress
	 */
	public InetAddress getLocalIpAddress()
	{
		return localIpAddress;
	}

   /**
    * @return the type
    */
   public ConnectionPointType getType()
   {
      return type;
   }

   /**
    * @return the historical
    */
   public boolean isHistorical()
   {
      return historical;
   }

   /**
    * @param historical the historical to set
    */
   public void setHistorical(boolean historical)
   {
      this.historical = historical;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ConnectionPoint [localNodeId=" + localNodeId + ", localInterfaceId=" + localInterfaceId + ", localMacAddress="
            + localMacAddress + ", localIpAddress=" + localIpAddress + ", nodeId=" + nodeId + ", interfaceId=" + interfaceId
            + ", interfaceIndex=" + interfaceIndex + ", type=" + type + ", data=" + data + "]";
   }
}
