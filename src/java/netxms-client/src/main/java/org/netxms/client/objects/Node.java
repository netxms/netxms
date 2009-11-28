/**
 * 
 */
package org.netxms.client.objects;

import org.netxms.base.*;
import org.netxms.client.NXCSession;

/**
 * This class represents NetXMS NODE objects.
 * 
 * @author Victor
 *
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

	// Node flags (user)
	public static final int NF_DISABLE_SNMP         = 0x01000000;
	public static final int NF_DISABLE_NXCP         = 0x02000000;
	public static final int NF_DISABLE_ICMP         = 0x04000000;
	public static final int NF_FORCE_ENCRYPTION     = 0x08000000;
	public static final int NF_DISABLE_STATUS_POLL  = 0x10000000;
	public static final int NF_DISABLE_CONF_POLL    = 0x20000000;
	public static final int NF_DISABLE_ROUTE_POLL   = 0x40000000;
	public static final int NF_DISABLE_DATA_COLLECT = 0x80000000;
	
	private int flags;
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
	private String systemDescription;
	
	/**
	 * @param msg
	 */
	public Node(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);

		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		nodeType = msg.getVariableAsInteger(NXCPCodes.VID_NODE_TYPE);
		requredPollCount = msg.getVariableAsInteger(NXCPCodes.VID_REQUIRED_POLLS);
		pollerNodeId = msg.getVariableAsInt64(NXCPCodes.VID_POLLER_NODE_ID);
		proxyNodeId = msg.getVariableAsInt64(NXCPCodes.VID_PROXY_NODE);
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
		snmpVersion = msg.getVariableAsInteger(NXCPCodes.VID_SNMP_VERSION);
		systemDescription = msg.getVariableAsString(NXCPCodes.VID_SYS_DESCRIPTION);
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
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
	 * @see org.netxms.client.NXCObject#getObjectClassName()
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
}
