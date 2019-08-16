/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.constants.AgentCompressionMode;
import org.netxms.client.constants.IcmpStatCollectionMode;
import org.netxms.client.constants.NodeType;
import org.netxms.client.constants.RackOrientation;

/**
 * Abstract base class for node objects.
 */
public abstract class AbstractNode extends DataCollectionTarget implements RackElement, ZoneMember, PollingTarget
{
	// SNMP versions
	public static final int SNMP_VERSION_1 = 0;
	public static final int SNMP_VERSION_2C = 1;
	public static final int SNMP_VERSION_3 = 3;
	
	// Agent authentication methods
	public static final int AGENT_AUTH_NONE = 0;
	public static final int AGENT_AUTH_PLAINTEXT = 1;
	public static final int AGENT_AUTH_MD5 = 2;
	public static final int AGENT_AUTH_SHA1 = 3;
	
	// Node capabilities
	public static final int NC_IS_SNMP                = 0x00000001;
	public static final int NC_IS_NATIVE_AGENT        = 0x00000002;
	public static final int NC_IS_BRIDGE              = 0x00000004;
	public static final int NC_IS_ROUTER              = 0x00000008;
	public static final int NC_IS_LOCAL_MGMT          = 0x00000010;
	public static final int NC_IS_PRINTER             = 0x00000020;
	public static final int NC_IS_OSPF                = 0x00000040;
	public static final int NC_IS_CPSNMP              = 0x00000080;
	public static final int NC_IS_CDP                 = 0x00000100;
	public static final int NC_IS_NDP                 = 0x00000200;
	public static final int NC_IS_LLDP                = 0x00000400;
	public static final int NC_IS_VRRP                = 0x00000800;
	public static final int NC_HAS_VLANS              = 0x00001000;
	public static final int NC_IS_8021X               = 0x00002000;
	public static final int NC_IS_STP                 = 0x00004000;
	public static final int NC_HAS_ENTITY_MIB         = 0x00008000;
	public static final int NC_HAS_IFXTABLE           = 0x00010000;
	public static final int NC_HAS_AGENT_IFXCOUNTERS  = 0x00020000;
	public static final int NC_HAS_WINPDH             = 0x00040000;
	public static final int NC_IS_WIFI_CONTROLLER     = 0x00080000;
	public static final int NC_IS_SMCLP               = 0x00100000;
	public static final int NC_IS_USER_AGENT_INSTALLED = 0x00400000;

	// Node flags
   public static final int NF_REMOTE_AGENT           = 0x00010000;
	public static final int NF_DISABLE_DISCOVERY_POLL = 0x00020000;
	public static final int NF_DISABLE_TOPOLOGY_POLL  = 0x00040000;
	public static final int NF_DISABLE_SNMP           = 0x00080000;
	public static final int NF_DISABLE_NXCP           = 0x00100000;
	public static final int NF_DISABLE_ICMP           = 0x00200000;
	public static final int NF_FORCE_ENCRYPTION       = 0x00400000;
   public static final int NF_DISABLE_ROUTE_POLL     = 0x00800000;
   public static final int NF_AGENT_OVER_TUNNEL_ONLY = 0x01000000;
   public static final int NF_SNMP_SETTINGS_LOCKED   = 0x02000000;
	
	// Node state flags
	public static final int NSF_AGENT_UNREACHABLE  = 0x00010000;
	public static final int NSF_SNMP_UNREACHABLE   = 0x00020000;
	public static final int NSF_CPSNMP_UNREACHABLE = 0x00040000;
	
	public static final int IFXTABLE_DEFAULT = 0;
	public static final int IFXTABLE_ENABLED = 1;
	public static final int IFXTABLE_DISABLED = 2;
	
	protected InetAddressEx primaryIP;
	protected String primaryName;
	protected int flags;
	protected int stateFlags;
	protected int capabilities;
	protected NodeType nodeType;
	protected String nodeSubType;
	protected String hypervisorType;
	protected String hypervisorInformation;
	protected int requredPollCount;
	protected long pollerNodeId;
	protected long agentProxyId;
	protected long snmpProxyId;
   protected long icmpProxyId;
	protected int agentPort;
	protected int agentAuthMethod;
	protected UUID agentId;
	protected AgentCacheMode agentCacheMode;
	protected AgentCompressionMode agentCompressionMode;
	protected String agentSharedSecret;
	protected String agentVersion;
	protected String platformName;
	protected String snmpAuthName;
	protected String snmpAuthPassword;
	protected String snmpPrivPassword;
	protected int snmpAuthMethod;
	protected int snmpPrivMethod;
	protected String snmpOID;
	protected int snmpVersion;
	protected int snmpPort;
	protected String snmpSysName;
   protected String snmpSysContact;
   protected String snmpSysLocation;
	protected String systemDescription;
	protected String lldpNodeId;
	protected int vrrpVersion;
	protected String driverName;
	protected String driverVersion;
	protected long zoneId;
	protected MacAddress bridgeBaseAddress;
	protected int ifXTablePolicy;
	protected Date bootTime;
	protected Date lastAgentCommTime;
	protected long rackId;
	protected UUID rackImageFront;
   protected UUID rackImageRear;
	protected short rackPosition;
	protected short rackHeight;
   protected RackOrientation rackOrientation;
   protected long chassisId;
   protected String sshLogin;
   protected String sshPassword;
   protected long sshProxyId;
   protected int portRowCount;
   protected int portNumberingScheme;
   protected IcmpStatCollectionMode icmpStatCollectionMode;
   protected List<InetAddress> icmpTargets;
   protected boolean icmpStatisticsCollected;
   protected int icmpLastResponseTime;
   protected int icmpMinResponseTime;
   protected int icmpMaxResponseTime;
   protected int icmpAverageResponseTime;
   protected int icmpPacketLoss;
	
	/**
	 * Create new node object.
	 * 
	 * @param id
	 * @param session
	 */
	public AbstractNode(long id, NXCSession session)
	{
		super(id, session);
	}

	/**
	 * Create node object from NXCP message.
	 * 
	 * @param msg
	 * @param session
	 */
	public AbstractNode(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);

		primaryIP = msg.getFieldAsInetAddressEx(NXCPCodes.VID_IP_ADDRESS);
		primaryName = msg.getFieldAsString(NXCPCodes.VID_PRIMARY_NAME);
		flags = msg.getFieldAsInt32(NXCPCodes.VID_FLAGS);
		stateFlags = msg.getFieldAsInt32(NXCPCodes.VID_STATE_FLAGS);
		capabilities = msg.getFieldAsInt32(NXCPCodes.VID_CAPABILITIES);
		nodeType = NodeType.getByValue(msg.getFieldAsInt16(NXCPCodes.VID_NODE_TYPE));
		nodeSubType = msg.getFieldAsString(NXCPCodes.VID_NODE_SUBTYPE);
      hypervisorType = msg.getFieldAsString(NXCPCodes.VID_HYPERVISOR_TYPE);
      hypervisorInformation = msg.getFieldAsString(NXCPCodes.VID_HYPERVISOR_INFO);
		requredPollCount = msg.getFieldAsInt32(NXCPCodes.VID_REQUIRED_POLLS);
		pollerNodeId = msg.getFieldAsInt64(NXCPCodes.VID_POLLER_NODE_ID);
		agentProxyId = msg.getFieldAsInt64(NXCPCodes.VID_AGENT_PROXY);
		snmpProxyId = msg.getFieldAsInt64(NXCPCodes.VID_SNMP_PROXY);
      icmpProxyId = msg.getFieldAsInt64(NXCPCodes.VID_ICMP_PROXY);
		agentPort = msg.getFieldAsInt32(NXCPCodes.VID_AGENT_PORT);
		agentAuthMethod = msg.getFieldAsInt32(NXCPCodes.VID_AUTH_METHOD);
		agentSharedSecret = msg.getFieldAsString(NXCPCodes.VID_SHARED_SECRET);
      agentCacheMode = AgentCacheMode.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_AGENT_CACHE_MODE));
      agentCompressionMode = AgentCompressionMode.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_AGENT_COMPRESSION_MODE));
		agentVersion = msg.getFieldAsString(NXCPCodes.VID_AGENT_VERSION);
		agentId = msg.getFieldAsUUID(NXCPCodes.VID_AGENT_ID);
		platformName = msg.getFieldAsString(NXCPCodes.VID_PLATFORM_NAME);
		snmpAuthName = msg.getFieldAsString(NXCPCodes.VID_SNMP_AUTH_OBJECT);
		snmpAuthPassword = msg.getFieldAsString(NXCPCodes.VID_SNMP_AUTH_PASSWORD);
		snmpPrivPassword = msg.getFieldAsString(NXCPCodes.VID_SNMP_PRIV_PASSWORD);
		int methods = msg.getFieldAsInt32(NXCPCodes.VID_SNMP_USM_METHODS);
		snmpAuthMethod = methods & 0xFF;
		snmpPrivMethod = methods >> 8;
		snmpOID = msg.getFieldAsString(NXCPCodes.VID_SNMP_OID);
		snmpPort = msg.getFieldAsInt32(NXCPCodes.VID_SNMP_PORT);
		snmpVersion = msg.getFieldAsInt32(NXCPCodes.VID_SNMP_VERSION);
		systemDescription = msg.getFieldAsString(NXCPCodes.VID_SYS_DESCRIPTION);
		snmpSysName = msg.getFieldAsString(NXCPCodes.VID_SYS_NAME);
      snmpSysContact = msg.getFieldAsString(NXCPCodes.VID_SYS_CONTACT);
      snmpSysLocation = msg.getFieldAsString(NXCPCodes.VID_SYS_LOCATION);
		lldpNodeId = msg.getFieldAsString(NXCPCodes.VID_LLDP_NODE_ID);
		vrrpVersion = msg.getFieldAsInt32(NXCPCodes.VID_VRRP_VERSION);
		driverName = msg.getFieldAsString(NXCPCodes.VID_DRIVER_NAME);
		driverVersion = msg.getFieldAsString(NXCPCodes.VID_DRIVER_VERSION);
		zoneId = msg.getFieldAsInt64(NXCPCodes.VID_ZONE_UIN);
		bridgeBaseAddress = new MacAddress(msg.getFieldAsBinary(NXCPCodes.VID_BRIDGE_BASE_ADDRESS));
		ifXTablePolicy = msg.getFieldAsInt32(NXCPCodes.VID_USE_IFXTABLE);
		rackId = msg.getFieldAsInt64(NXCPCodes.VID_RACK_ID);
		rackImageFront = msg.getFieldAsUUID(NXCPCodes.VID_RACK_IMAGE_FRONT);
      rackImageRear = msg.getFieldAsUUID(NXCPCodes.VID_RACK_IMAGE_REAR);
		rackPosition = msg.getFieldAsInt16(NXCPCodes.VID_RACK_POSITION);
      rackHeight = msg.getFieldAsInt16(NXCPCodes.VID_RACK_HEIGHT);
      chassisId = msg.getFieldAsInt64(NXCPCodes.VID_CHASSIS_ID);
      sshLogin = msg.getFieldAsString(NXCPCodes.VID_SSH_LOGIN);
      sshPassword = msg.getFieldAsString(NXCPCodes.VID_SSH_PASSWORD);
      sshProxyId = msg.getFieldAsInt64(NXCPCodes.VID_SSH_PROXY);
      portRowCount = msg.getFieldAsInt16(NXCPCodes.VID_PORT_ROW_COUNT);
      portNumberingScheme = msg.getFieldAsInt16(NXCPCodes.VID_PORT_NUMBERING_SCHEME);
      rackOrientation = RackOrientation.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_RACK_ORIENTATION));
      icmpStatCollectionMode = IcmpStatCollectionMode.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_ICMP_COLLECTION_MODE));
      icmpStatisticsCollected = msg.getFieldAsBoolean(NXCPCodes.VID_HAS_ICMP_DATA);
      icmpAverageResponseTime = msg.getFieldAsInt32(NXCPCodes.VID_ICMP_AVG_RESPONSE_TIME);
      icmpLastResponseTime = msg.getFieldAsInt32(NXCPCodes.VID_ICMP_LAST_RESPONSE_TIME);
      icmpMaxResponseTime = msg.getFieldAsInt32(NXCPCodes.VID_ICMP_MAX_RESPONSE_TIME);
      icmpMinResponseTime = msg.getFieldAsInt32(NXCPCodes.VID_ICMP_MIN_RESPONSE_TIME);
      icmpPacketLoss = msg.getFieldAsInt32(NXCPCodes.VID_ICMP_PACKET_LOSS);
      
      int count = msg.getFieldAsInt32(NXCPCodes.VID_ICMP_TARGET_COUNT);
      if (count > 0)
      {
         icmpTargets = new ArrayList<InetAddress>(count);
         long fieldId = NXCPCodes.VID_ICMP_TARGET_LIST_BASE;
         for(int i = 0; i < count; i++)
            icmpTargets.add(msg.getFieldAsInetAddress(fieldId++));
      }
      
		long bootTimeSeconds = msg.getFieldAsInt64(NXCPCodes.VID_BOOT_TIME);
		bootTime = (bootTimeSeconds > 0) ? new Date(bootTimeSeconds * 1000) : null;

      long commTimeSeconds = msg.getFieldAsInt64(NXCPCodes.VID_AGENT_COMM_TIME);
      lastAgentCommTime = (commTimeSeconds > 0) ? new Date(commTimeSeconds * 1000) : null;
	}

	/**
	 * @return Flags
	 */
	public int getFlags()
	{
		return flags;
	}

   /**
    * @return Flags
    */
   public int getCapabilities()
   {
      return capabilities;
   }

	/**
	 * @return Runtime flags
	 */
	public int getStateFlags()
	{
		return stateFlags;
	}

	/**
	 * @return the nodeType
	 */
	public NodeType getNodeType()
	{
		return nodeType;
	}

	/**
    * @return the nodeSubType
    */
   public String getNodeSubType()
   {
      return nodeSubType;
   }
   
   /**
    * Check if this node is a virtual node (either virtual machine or container)
    * 
    * @return true if this node is a virtual node
    */
   public boolean isVirtual()
   {
      return (nodeType == NodeType.VIRTUAL) || (nodeType == NodeType.CONTAINER);
   }

   /**
    * Get hypervisor type.
    * 
    * @return hypervisor type string or empty string if unknown or not applicable 
    */
   public String getHypervisorType()
   {
      return (hypervisorType != null) ? hypervisorType : "";
   }

   /**
    * Get additional hypervisor information (product, version, etc.).
    * 
    * @return hypervisor information string or empty string if unknown or not applicable 
    */
   public String getHypervisorInformation()
   {
      return (hypervisorInformation != null) ? hypervisorInformation : "";
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
	public long getAgentProxyId()
	{
		return agentProxyId;
	}

	/**
	 * @return the snmpProxyId
	 */
	public long getSnmpProxyId()
	{
		return snmpProxyId;
	}

   /**
    * @return the icmpProxyId
    */
   public long getIcmpProxyId()
   {
      return icmpProxyId;
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
    * @return the agentCacheMode
    */
   public AgentCacheMode getAgentCacheMode()
   {
      return agentCacheMode;
   }

   /**
    * @return the agentCompressionMode
    */
   public AgentCompressionMode getAgentCompressionMode()
   {
      return agentCompressionMode;
   }

   /**
	 * @return the agentVersion
	 */
	public String getAgentVersion()
	{
		return agentVersion;
	}

	/**
    * @return the agentId
    */
   public UUID getAgentId()
   {
      return agentId;
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
		return (capabilities & NC_IS_NATIVE_AGENT) != 0;
	}

	/**
	 * 
	 * @return true if node has SNMP agent
	 */
	public boolean hasSnmpAgent()
	{
		return (capabilities & NC_IS_SNMP) != 0;
	}

	/**
	 * 
	 * @return true if node is a management server
	 */
	public boolean isManagementServer()
	{
		return (capabilities & NC_IS_LOCAL_MGMT) != 0;
	}

	/**
	 * 
	 * @return true if node is VRRP capable
	 */
	public boolean isVrrpSupported()
	{
		return (capabilities & NC_IS_VRRP) != 0;
	}

	/**
	 * 
	 * @return true if node supports 802.1x
	 */
	public boolean is8021xSupported()
	{
		return (capabilities & NC_IS_8021X) != 0;
	}

	/**
	 * 
	 * @return true if node supports spanning tree MIB
	 */
	public boolean isSpanningTreeSupported()
	{
		return (capabilities & NC_IS_STP) != 0;
	}

	/**
	 * 
	 * @return true if node supports ENTITY-MIB
	 */
	public boolean isEntityMibSupported()
	{
		return (capabilities & NC_HAS_ENTITY_MIB) != 0;
	}

	/**
	 * 
	 * @return true if node supports ifXTable
	 */
	public boolean isIfXTableSupported()
	{
		return (capabilities & NC_HAS_IFXTABLE) != 0;
	}

	/**
	 * 
	 * @return true if agent on node supports 64-bit interface counters
	 */
	public boolean isAgentIfXCountersSupported()
	{
		return (capabilities & NC_HAS_AGENT_IFXCOUNTERS) != 0;
	}

   /**
    * Check for bridge capabilities flag
    * 
    * @return true if node is has bridge capabilities
    */
   public boolean isBridge()
   {
      return (capabilities & NC_IS_BRIDGE) != 0;
   }

	/**
	 * 
	 * @return true if node is a wireless network controller
	 */
	public boolean isWirelessController()
	{
		return (capabilities & NC_IS_WIFI_CONTROLLER) != 0;
	}

	/**
	 * @return the snmpPort
	 */
	public int getSnmpPort()
	{
		return snmpPort;
	}

	/**
    * Get SNMP system name (value of sysName MIB entry)
    * 
	 * @return the snmpSysName
	 */
	public String getSnmpSysName()
	{
		return snmpSysName;
	}

	/**
    * Get SNMP system contact (value of sysContact MIB entry)
    * 
	 * @return SNMP system contact (value of sysContact MIB entry)
	 */
	public String getSnmpSysContact()
   {
      return snmpSysContact;
   }

   /**
    * Get SNMP system location (value of sysLocation MIB entry)
    * 
    * @return SNMP system location (value of sysLocation MIB entry)
    */
   public String getSnmpSysLocation()
   {
      return snmpSysLocation;
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

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.ZoneMember#getZoneId()
	 */
	@Override
	public long getZoneId()
	{
		return zoneId;
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.client.objects.ZoneMember#getZoneName()
	 */
	@Override
	public String getZoneName()
	{
	   Zone zone = session.findZone(zoneId);
	   return (zone != null) ? zone.getObjectName() : Long.toString(zoneId);
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

	/**
	 * Get MAC address of interface with node's primary IP
	 * 
	 * @return MAC address of interface with node's primary IP
	 */
	public MacAddress getPrimaryMAC()
	{
		for(AbstractObject o : getAllChildren(AbstractObject.OBJECT_INTERFACE))
		{
			Interface iface = (Interface)o;
			if (iface.isLoopback() || (iface.getMacAddress() == null))
				continue;
			
			if (iface.hasAddress(primaryIP))
			{
				return iface.getMacAddress();
			}
		}
		return null;
	}

   /**
    * @return the bootTime
    */
   public Date getBootTime()
   {
      return bootTime;
   }
   
   /**
    * Get primary IP address
    * 
    * @return primary IP address
    */
   public InetAddressEx getPrimaryIP()
   {
      return primaryIP;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRackId()
    */
   @Override
   public long getRackId()
   {
      return rackId;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getFrontRackImage()
    */
   @Override
   public UUID getFrontRackImage()
   {
      return rackImageFront;
   }
   
   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRearRackImage()
    */
   @Override
   public UUID getRearRackImage()
   {
      return rackImageRear;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRackPosition()
    */
   @Override
   public short getRackPosition()
   {
      return rackPosition;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRackHeight()
    */
   @Override
   public short getRackHeight()
   {
      return rackHeight;
   }

   /**
    * Get last agent communications time
    * 
    * @return last agent communications time
    */
   public Date getLastAgentCommTime()
   {
      return lastAgentCommTime;
   }

   /**
    * @return the chassisId
    */
   public long getChassisId()
   {
      return chassisId;
   }

   /**
    * @return the sshLogin
    */
   public String getSshLogin()
   {
      return sshLogin;
   }

   /**
    * @return the sshPassword
    */
   public String getSshPassword()
   {
      return sshPassword;
   }

   /**
    * @return the sshProxyId
    */
   public long getSshProxyId()
   {
      return sshProxyId;
   }
   
   /**
    * Get number of rows for physical ports
    * 
    * @return number of rows for physical ports
    */
   public int getPortRowCount()
   {
      return portRowCount;
   }
   
   /**
    * Get physical port numbering scheme
    * 
    * @return physical port numbering scheme
    */
   public int getPortNumberingScheme()
   {
      return portNumberingScheme;
   }
   
   /**
    * @return the icmpStatCollectionMode
    */
   public IcmpStatCollectionMode getIcmpStatCollectionMode()
   {
      return icmpStatCollectionMode;
   }

   /**
    * @return the icmpTargets
    */
   public InetAddress[] getIcmpTargets()
   {
      return (icmpTargets != null) ? icmpTargets.toArray(new InetAddress[icmpTargets.size()]) : new InetAddress[0];
   }

   /**
    * Check if ICMP statistics is collected for this node.
    * 
    * @return true if ICMP statistics is collected for this node
    */
   public boolean isIcmpStatisticsCollected()
   {
      return icmpStatisticsCollected;
   }

   /**
    * @return the icmpLastResponseTime
    */
   public int getIcmpLastResponseTime()
   {
      return icmpLastResponseTime;
   }

   /**
    * @return the icmpMinResponseTime
    */
   public int getIcmpMinResponseTime()
   {
      return icmpMinResponseTime;
   }

   /**
    * @return the icmpMaxResponseTime
    */
   public int getIcmpMaxResponseTime()
   {
      return icmpMaxResponseTime;
   }

   /**
    * @return the icmpAverageResponseTime
    */
   public int getIcmpAverageResponseTime()
   {
      return icmpAverageResponseTime;
   }

   /**
    * @return the icmpPacketLoss
    */
   public int getIcmpPacketLoss()
   {
      return icmpPacketLoss;
   }

   /**
    * Get node's interface by interface index
    * 
    * @param ifIndex interface index
    * @return corresponding interface object or null
    */
   public Interface getInterfaceByIndex(int ifIndex)
   {
      for(AbstractObject i : getAllChildren(AbstractObject.OBJECT_INTERFACE))
      {
         if (((Interface)i).getIfIndex() == ifIndex)
            return (Interface)i;
      }
      return null;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, primaryName);
      addString(strings, primaryIP.getHostAddress());
      addString(strings, nodeSubType);
      addString(strings, snmpAuthName);
      addString(strings, snmpOID);
      addString(strings, snmpSysContact);
      addString(strings, snmpSysLocation);
      addString(strings, snmpSysName);
      addString(strings, sshLogin);
      addString(strings, systemDescription);
      addString(strings, platformName);
      return strings;
   }
   
   /* (non-Javadoc)
    * @see org.netxms.client.objects.RackElement#getRackOrientation()
    */
   public RackOrientation getRackOrientation()
   {
      return rackOrientation;
   }
   
   /**
    * Check node state flags for "agent unreachable" flag.
    * 
    * @return true if agent is reachable (flag is cleared)
    */
   public boolean isAgentReachable()
   {
      return (stateFlags & NSF_AGENT_UNREACHABLE) == 0;
   }

   /**
    * Check node state flags for "SNMP unreachable" flag.
    * 
    * @return true if SNMP is reachable (flag is cleared)
    */
   public boolean isSnmpReachable()
   {
      return (stateFlags & NSF_SNMP_UNREACHABLE) == 0;
   }
   
   /**
    * Check node flags for "SNMP settings locked" flag.
    * 
    * @return true if SNMP settings are locked (flag is set)
    */
   public boolean isSnmpSettingsLocked()
   {
      return (flags & NF_SNMP_SETTINGS_LOCKED) != 0;      
   }
}
