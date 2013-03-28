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
package org.netxms.client;

import java.net.InetAddress;
import java.net.UnknownHostException;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkService;

/**
 * NetXMS object creation data
 */
public class NXCObjectCreationData
{
	// Creation flags
	public static int CF_DISABLE_ICMP = 0x0001;
	public static int CF_DISABLE_NXCP = 0x0002;
	public static int CF_DISABLE_SNMP = 0x0004;
	public static int CF_CREATE_UNMANAGED = 0x0008;
	
	private int objectClass;
	private String name;
	private long parentId;
	private String comments;
	private int creationFlags;
	private String primaryName;
	private int agentPort;
	private int snmpPort;
	private InetAddress ipAddress;
	private InetAddress ipNetMask;
	private long agentProxyId;
	private long snmpProxyId;
	private int mapType;
	private long seedObjectId;
	private long zoneId;
	private int serviceType;
	private int ipProtocol;
	private int ipPort;
	private String request;
	private String response;
	private long linkedNodeId;
	private boolean template;
	private MacAddress macAddress;
	private int ifIndex;
	private int ifType;
	private int slot;
	private int port;
	private boolean physicalPort;
	private boolean createStatusDci;
	private String deviceId;
	
	/**
	 * Constructor.
	 * 
	 * @param objectClass Class of new object (one of NXCObject.OBJECT_xxx constants)
	 * @see AbstractObject
	 * @param name Name of new object
	 * @param parentId Parent object ID
	 */
	public NXCObjectCreationData(final int objectClass, final String name, final long parentId)
	{
		this.objectClass = objectClass;
		this.name = name;
		this.parentId = parentId;
		
		try
		{
			ipAddress = InetAddress.getByName("127.0.0.1");
			ipNetMask = InetAddress.getByName("0.0.0.0");
		}
		catch(UnknownHostException e)
		{
		}
		
		primaryName = null;
		agentPort = 0;
		snmpPort = 0;
		comments = null;
		creationFlags = 0;
		agentProxyId = 0;
		snmpProxyId = 0;
		mapType = 0;
		seedObjectId = 0;
		zoneId = 0;
		serviceType = NetworkService.CUSTOM;
		ipProtocol = 6;
		ipPort = 80;
		request = "";
		response = "";
		linkedNodeId = 0;
		template = false;
		macAddress = new MacAddress();
		ifIndex = 0;
		ifType = 1;
		slot = 0;
		port = 0;
		physicalPort = false;
		createStatusDci = false;
	}

	/**
	 * @return the objectClass
	 */
	public int getObjectClass()
	{
		return objectClass;
	}

	/**
	 * @param objectClass the objectClass to set
	 */
	public void setObjectClass(int objectClass)
	{
		this.objectClass = objectClass;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the parentId
	 */
	public long getParentId()
	{
		return parentId;
	}

	/**
	 * @param parentId the parentId to set
	 */
	public void setParentId(long parentId)
	{
		this.parentId = parentId;
	}

	/**
	 * @return the comments
	 */
	public String getComments()
	{
		return comments;
	}

	/**
	 * @param comments the comments to set
	 */
	public void setComments(String comments)
	{
		this.comments = comments;
	}

	/**
	 * @return the creationFlags
	 */
	public int getCreationFlags()
	{
		return creationFlags;
	}

	/**
	 * @param creationFlags Node creation flags (combination of NXCObjectCreationData.CF_xxx constants)
	 */
	public void setCreationFlags(int creationFlags)
	{
		this.creationFlags = creationFlags;
	}

	/**
	 * @return the ipAddress
	 */
	public InetAddress getIpAddress()
	{
		return ipAddress;
	}

	/**
	 * @param ipAddress the ipAddress to set
	 */
	public void setIpAddress(InetAddress ipAddress)
	{
		this.ipAddress = ipAddress;
	}

	/**
	 * @return the ipNetMask
	 */
	public InetAddress getIpNetMask()
	{
		return ipNetMask;
	}

	/**
	 * @param ipNetMask the ipNetMask to set
	 */
	public void setIpNetMask(InetAddress ipNetMask)
	{
		this.ipNetMask = ipNetMask;
	}

	/**
	 * @return the agentProxyId
	 */
	public long getAgentProxyId()
	{
		return agentProxyId;
	}

	/**
	 * @param agentProxyId the agentProxyId to set
	 */
	public void setAgentProxyId(long agentProxyId)
	{
		this.agentProxyId = agentProxyId;
	}

	/**
	 * @return the snmpProxyId
	 */
	public long getSnmpProxyId()
	{
		return snmpProxyId;
	}

	/**
	 * @param snmpProxyId the snmpProxyId to set
	 */
	public void setSnmpProxyId(long snmpProxyId)
	{
		this.snmpProxyId = snmpProxyId;
	}

	/**
	 * @return the mapType
	 */
	public int getMapType()
	{
		return mapType;
	}

	/**
	 * @param mapType the mapType to set
	 */
	public void setMapType(int mapType)
	{
		this.mapType = mapType;
	}

	/**
	 * @return the seedObjectId
	 */
	public long getSeedObjectId()
	{
		return seedObjectId;
	}

	/**
	 * @param seedObjectId the seedObjectId to set
	 */
	public void setSeedObjectId(long seedObjectId)
	{
		this.seedObjectId = seedObjectId;
	}

	/**
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}

	/**
	 * @param zoneId the zoneId to set
	 */
	public void setZoneId(long zoneId)
	{
		this.zoneId = zoneId;
	}

	/**
	 * @return the serviceType
	 */
	public int getServiceType()
	{
		return serviceType;
	}

	/**
	 * @param serviceType the serviceType to set
	 */
	public void setServiceType(int serviceType)
	{
		this.serviceType = serviceType;
	}

	/**
	 * @return the ipProtocol
	 */
	public int getIpProtocol()
	{
		return ipProtocol;
	}

	/**
	 * @param ipProtocol the ipProtocol to set
	 */
	public void setIpProtocol(int ipProtocol)
	{
		this.ipProtocol = ipProtocol;
	}

	/**
	 * @return the ipPort
	 */
	public int getIpPort()
	{
		return ipPort;
	}

	/**
	 * @param ipPort the ipPort to set
	 */
	public void setIpPort(int ipPort)
	{
		this.ipPort = ipPort;
	}

	/**
	 * @return the request
	 */
	public String getRequest()
	{
		return request;
	}

	/**
	 * @param request the request to set
	 */
	public void setRequest(String request)
	{
		this.request = request;
	}

	/**
	 * @return the response
	 */
	public String getResponse()
	{
		return response;
	}

	/**
	 * @param response the response to set
	 */
	public void setResponse(String response)
	{
		this.response = response;
	}

	/**
	 * @return the linkedNodeId
	 */
	public long getLinkedNodeId()
	{
		return linkedNodeId;
	}

	/**
	 * @param linkedNodeId the linkedNodeId to set
	 */
	public void setLinkedNodeId(long linkedNodeId)
	{
		this.linkedNodeId = linkedNodeId;
	}

	/**
	 * @return the primaryName
	 */
	public String getPrimaryName()
	{
		return primaryName;
	}

	/**
	 * @param primaryName the primaryName to set
	 */
	public void setPrimaryName(String primaryName)
	{
		this.primaryName = primaryName;
	}

	/**
	 * @return the template
	 */
	public boolean isTemplate()
	{
		return template;
	}

	/**
	 * @param template the template to set
	 */
	public void setTemplate(boolean template)
	{
		this.template = template;
	}

	/**
	 * @return the macAddress
	 */
	public MacAddress getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @param macAddress the macAddress to set
	 */
	public void setMacAddress(MacAddress macAddress)
	{
		this.macAddress = macAddress;
	}

	/**
	 * @return the ifIndex
	 */
	public int getIfIndex()
	{
		return ifIndex;
	}

	/**
	 * @param ifIndex the ifIndex to set
	 */
	public void setIfIndex(int ifIndex)
	{
		this.ifIndex = ifIndex;
	}

	/**
	 * @return the ifType
	 */
	public int getIfType()
	{
		return ifType;
	}

	/**
	 * @param ifType the ifType to set
	 */
	public void setIfType(int ifType)
	{
		this.ifType = ifType;
	}

	/**
	 * @return the slot
	 */
	public int getSlot()
	{
		return slot;
	}

	/**
	 * @param slot the slot to set
	 */
	public void setSlot(int slot)
	{
		this.slot = slot;
	}

	/**
	 * @return the port
	 */
	public int getPort()
	{
		return port;
	}

	/**
	 * @param port the port to set
	 */
	public void setPort(int port)
	{
		this.port = port;
	}

	/**
	 * @return the physicalPort
	 */
	public boolean isPhysicalPort()
	{
		return physicalPort;
	}

	/**
	 * @param physicalPort the physicalPort to set
	 */
	public void setPhysicalPort(boolean physicalPort)
	{
		this.physicalPort = physicalPort;
	}

	/**
	 * @return the createStatusDci
	 */
	public boolean isCreateStatusDci()
	{
		return createStatusDci;
	}

	/**
	 * @param createStatusDci the createStatusDci to set
	 */
	public void setCreateStatusDci(boolean createStatusDci)
	{
		this.createStatusDci = createStatusDci;
	}

	/**
	 * @return the agentPort
	 */
	public int getAgentPort()
	{
		return agentPort;
	}

	/**
	 * @param agentPort the agentPort to set
	 */
	public void setAgentPort(int agentPort)
	{
		this.agentPort = agentPort;
	}

	/**
	 * @return the snmpPort
	 */
	public int getSnmpPort()
	{
		return snmpPort;
	}

	/**
	 * @param snmpPort the snmpPort to set
	 */
	public void setSnmpPort(int snmpPort)
	{
		this.snmpPort = snmpPort;
	}

	/**
	 * @return the deviceId
	 */
	public final String getDeviceId()
	{
		return deviceId;
	}

	/**
	 * @param deviceId the deviceId to set
	 */
	public final void setDeviceId(String deviceId)
	{
		this.deviceId = deviceId;
	}
}
