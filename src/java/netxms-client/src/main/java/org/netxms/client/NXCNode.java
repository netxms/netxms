/**
 * 
 */
package org.netxms.client;

import org.netxms.base.*;

/**
 * This class represents NetXMS NODE objects.
 * 
 * @author Victor
 *
 */
public class NXCNode extends NXCObject
{
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
	private String snmpCommunityString;
	private String snmpOID;
	private int snmpVersion;
	private String systemDescription;
	
	/**
	 * @param msg
	 */
	public NXCNode(NXCPMessage msg, NXCSession session)
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
		snmpCommunityString = msg.getVariableAsString(NXCPCodes.VID_COMMUNITY_STRING);
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
	 * @return the snmpCommunityString
	 */
	public String getSnmpCommunityString()
	{
		return snmpCommunityString;
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
}
