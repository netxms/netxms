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

import org.netxms.base.*;
import org.netxms.client.MacAddress;
import org.netxms.client.NXCSession;

/**
 * This class represents NetXMS NODE objects.
 */
public class Node extends GenericObject
{
	public static final int SNMP_VERSION_1 = 0;
	public static final int SNMP_VERSION_2C = 1;
	public static final int SNMP_VERSION_3 = 3;
	
	public static final int AGENT_AUTH_NONE = 0;
	public static final int AGENT_AUTH_PLAINTEXT = 1;
	public static final int AGENT_AUTH_MD5 = 2;
	public static final int AGENT_AUTH_SHA1 = 3;

	// Node flags (system)
	public static final int NF_IS_SNMP              = 0x00000001;
	public static final int NF_IS_NATIVE_AGENT      = 0x00000002;
	public static final int NF_IS_BRIDGE            = 0x00000004;
	public static final int NF_IS_ROUTER            = 0x00000008;
	public static final int NF_IS_LOCAL_MGMT        = 0x00000010;
	public static final int NF_IS_PRINTER           = 0x00000020;
	public static final int NF_IS_OSPF              = 0x00000040;
	public static final int NF_BEHIND_NAT           = 0x00000080;
	public static final int NF_IS_CPSNMP            = 0x00000100;
	public static final int NF_IS_CDP               = 0x00000200;
	public static final int NF_IS_SONMP             = 0x00000400;
	public static final int NF_IS_LLDP              = 0x00000800;
	public static final int NF_IS_VRRP              = 0x00001000;
	public static final int NF_IS_8021X             = 0x00002000;
	public static final int NF_IS_STP               = 0x00004000;

	// Node flags (user)
	public static final int NF_DISABLE_SNMP         = 0x01000000;
	public static final int NF_DISABLE_NXCP         = 0x02000000;
	public static final int NF_DISABLE_ICMP         = 0x04000000;
	public static final int NF_FORCE_ENCRYPTION     = 0x08000000;
	public static final int NF_DISABLE_STATUS_POLL  = 0x10000000;
	public static final int NF_DISABLE_CONF_POLL    = 0x20000000;
	public static final int NF_DISABLE_ROUTE_POLL   = 0x40000000;
	public static final int NF_DISABLE_DATA_COLLECT = 0x80000000;
	
	// Node flags (runtime)
	public static final int NDF_UNREACHABLE         = 0x000000004;
	public static final int NDF_AGENT_UNREACHABLE   = 0x000000008;
	public static final int NDF_SNMP_UNREACHABLE    = 0x000000010;
	public static final int NDF_CPSNMP_UNREACHABLE  = 0x000000200;
	public static final int NDF_POLLING_DISABLED    = 0x000000800;
	
	// ixXTable usage policy
	public static final int IFXTABLE_DEFAULT        = 0;
	public static final int IFXTABLE_ENABLED        = 1;
	public static final int IFXTABLE_DISABLED       = 2;
	
	private String primaryName;
	private int flags;
	private int runtimeFlags;
	private int nodeType;
	private int requredPollCount;
	private long pollerNodeId;
	private long proxyNodeId;
	private long snmpProxyId;
	private int agentPort;
	private int agentAuthMethod;
	private String agentSharedSecret;
	private String agentVersion;
	private String platformName;
	private String snmpAuthName;
	private String snmpAuthPassword;
	private String snmpPrivPassword;
	private int snmpAuthMethod;
	private int snmpPrivMethod;
	private String snmpOID;
	private int snmpVersion;
	private int snmpPort;
	private String snmpSysName;
	private String systemDescription;
	private String lldpNodeId;
	private int vrrpVersion;
	private String driverName;
	private String driverVersion;
	private long zoneId;
	private MacAddress bridgeBaseAddress;
	private int ifXTablePolicy;
	
	/**
	 * @param msg
	 */
	public Node(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);

		primaryName = msg.getVariableAsString(NXCPCodes.VID_PRIMARY_NAME);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		runtimeFlags = msg.getVariableAsInteger(NXCPCodes.VID_RUNTIME_FLAGS);
		nodeType = msg.getVariableAsInteger(NXCPCodes.VID_NODE_TYPE);
		requredPollCount = msg.getVariableAsInteger(NXCPCodes.VID_REQUIRED_POLLS);
		pollerNodeId = msg.getVariableAsInt64(NXCPCodes.VID_POLLER_NODE_ID);
		proxyNodeId = msg.getVariableAsInt64(NXCPCodes.VID_AGENT_PROXY);
		snmpProxyId = msg.getVariableAsInt64(NXCPCodes.VID_SNMP_PROXY);
		agentPort = msg.getVariableAsInteger(NXCPCodes.VID_AGENT_PORT);
		agentAuthMethod = msg.getVariableAsInteger(NXCPCodes.VID_AUTH_METHOD);
		agentSharedSecret = msg.getVariableAsString(NXCPCodes.VID_SHARED_SECRET);
		agentVersion = msg.getVariableAsString(NXCPCodes.VID_AGENT_VERSION);
		platformName = msg.getVariableAsString(NXCPCodes.VID_PLATFORM_NAME);
		snmpAuthName = msg.getVariableAsString(NXCPCodes.VID_SNMP_AUTH_OBJECT);
		snmpAuthPassword = msg.getVariableAsString(NXCPCodes.VID_SNMP_AUTH_PASSWORD);
		snmpPrivPassword = msg.getVariableAsString(NXCPCodes.VID_SNMP_PRIV_PASSWORD);
		int methods = msg.getVariableAsInteger(NXCPCodes.VID_SNMP_USM_METHODS);
		snmpAuthMethod = methods & 0xFF;
		snmpPrivMethod = methods >> 8;
		snmpOID = msg.getVariableAsString(NXCPCodes.VID_SNMP_OID);
		snmpPort = msg.getVariableAsInteger(NXCPCodes.VID_SNMP_PORT);
		snmpVersion = msg.getVariableAsInteger(NXCPCodes.VID_SNMP_VERSION);
		systemDescription = msg.getVariableAsString(NXCPCodes.VID_SYS_DESCRIPTION);
		snmpSysName = msg.getVariableAsString(NXCPCodes.VID_SYS_NAME);
		lldpNodeId = msg.getVariableAsString(NXCPCodes.VID_LLDP_NODE_ID);
		vrrpVersion = msg.getVariableAsInteger(NXCPCodes.VID_VRRP_VERSION);
		driverName = msg.getVariableAsString(NXCPCodes.VID_DRIVER_NAME);
		driverVersion = msg.getVariableAsString(NXCPCodes.VID_DRIVER_VERSION);
		zoneId = msg.getVariableAsInt64(NXCPCodes.VID_ZONE_ID);
		bridgeBaseAddress = new MacAddress(msg.getVariableAsBinary(NXCPCodes.VID_BRIDGE_BASE_ADDRESS));
		ifXTablePolicy = msg.getVariableAsInteger(NXCPCodes.VID_USE_IFXTABLE);
	}

	/**
	 * @return Flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @return Runtime flags
	 */
	public int getRuntimeFlags()
	{
		return runtimeFlags;
	}

	/**
	 * @return the nodeType
	 */
	public int getNodeType()
	{
		return nodeType;
	}

	/**
	 * @return the requredPollCount
	 */
	public int getRequredPollCount()
	{
		return requredPollCount;
	}

	/**
	 * @return the pollerNodeId
	 */
	public long getPollerNodeId()
	{
		return pollerNodeId;
	}

	/**
	 * @return the proxyNodeId
	 */
	public long getProxyNodeId()
	{
		return proxyNodeId;
	}

	/**
	 * @return the snmpProxyId
	 */
	public long getSnmpProxyId()
	{
		return snmpProxyId;
	}

	/**
	 * @return the agentPort
	 */
	public int getAgentPort()
	{
		return agentPort;
	}

	/**
	 * @return the agentAuthMethod
	 */
	public int getAgentAuthMethod()
	{
		return agentAuthMethod;
	}

	/**
	 * @return the agentSharedSecret
	 */
	public String getAgentSharedSecret()
	{
		return agentSharedSecret;
	}

	/**
	 * @return the agentVersion
	 */
	public String getAgentVersion()
	{
		return agentVersion;
	}

	/**
	 * @return the platformName
	 */
	public String getPlatformName()
	{
		return platformName;
	}

	/**
	 * Get SNMP authentication name - community string for version 1 and 2c, or user name for version 3.
	 * 
	 * @return SNMP authentication name
	 */
	public String getSnmpAuthName()
	{
		return snmpAuthName;
	}

	/**
	 * @return the snmpOID
	 */
	public String getSnmpOID()
	{
		return snmpOID;
	}

	/**
	 * @return the snmpVersion
	 */
	public int getSnmpVersion()
	{
		return snmpVersion;
	}

	/**
	 * @return the systemDescription
	 */
	public String getSystemDescription()
	{
		return systemDescription;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Node";
	}

	/**
	 * Get SNMP authentication password.
	 * 
	 * @return SNMP authentication password
	 */
	public String getSnmpAuthPassword()
	{
		return snmpAuthPassword;
	}

	/**
	 * Get SNMP privacy password.
	 * 
	 * @return SNMP privacy password
	 */
	public String getSnmpPrivPassword()
	{
		return snmpPrivPassword;
	}

	/**
	 * Get SNMP authentication method
	 * 
	 * @return SNMP authentication method
	 */
	public int getSnmpAuthMethod()
	{
		return snmpAuthMethod;
	}

	/**
	 * Get SNMP privacy (encryption) method
	 * 
	 * @return SNMP privacy method
	 */
	public int getSnmpPrivMethod()
	{
		return snmpPrivMethod;
	}
	
	/**
	 * 
	 * @return true if node has NetXMS agent
	 */
	public boolean hasAgent()
	{
		return (flags & NF_IS_NATIVE_AGENT) != 0;
	}
	
	/**
	 * 
	 * @return true if node has SNMP agent
	 */
	public boolean hasSnmpAgent()
	{
		return (flags & NF_IS_SNMP) != 0;
	}
	
	/**
	 * 
	 * @return true if node is a management server
	 */
	public boolean isManagementServer()
	{
		return (flags & NF_IS_LOCAL_MGMT) != 0;
	}

	/**
	 * 
	 * @return true if node is VRRP capable
	 */
	public boolean isVrrpSupported()
	{
		return (flags & NF_IS_VRRP) != 0;
	}

	/**
	 * 
	 * @return true if node supports 802.1x
	 */
	public boolean is8021xSupported()
	{
		return (flags & NF_IS_8021X) != 0;
	}

	/**
	 * 
	 * @return true if node supports spanning tree MIB
	 */
	public boolean isSpanningTreeSupported()
	{
		return (flags & NF_IS_STP) != 0;
	}

	/**
	 * @return the snmpPort
	 */
	public int getSnmpPort()
	{
		return snmpPort;
	}

	/**
	 * @return the snmpSysName
	 */
	public String getSnmpSysName()
	{
		return snmpSysName;
	}

	/**
	 * @return the lldpNodeId
	 */
	public String getLldpNodeId()
	{
		return lldpNodeId;
	}

	/**
	 * @return the vrrpVersion
	 */
	protected int getVrrpVersion()
	{
		return vrrpVersion;
	}

	/**
	 * @return the driverName
	 */
	public String getDriverName()
	{
		return driverName;
	}

	/**
	 * @return the driverVersion
	 */
	public String getDriverVersion()
	{
		return driverVersion;
	}

	/**
	 * @return the zoneId
	 */
	public long getZoneId()
	{
		return zoneId;
	}

	/**
	 * @return the bridgeBaseAddress
	 */
	public MacAddress getBridgeBaseAddress()
	{
		return bridgeBaseAddress;
	}

	/**
	 * @return the ifXTablePolicy
	 */
	public int getIfXTablePolicy()
	{
		return ifXTablePolicy;
	}

	/**
	 * @return the primaryName
	 */
	public String getPrimaryName()
	{
		return primaryName;
	}
}
