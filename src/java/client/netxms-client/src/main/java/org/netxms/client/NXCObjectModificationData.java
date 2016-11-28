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
package org.netxms.client;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.InetAddress;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.netxms.base.GeoLocation;
import org.netxms.base.InetAddressEx;
import org.netxms.base.PostalAddress;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ConditionDciInfo;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.objects.ClusterResource;

/**
 * This class is used to hold data for NXCSession.modifyObject()
 */
public class NXCObjectModificationData
{
	// Modification flags
	public static final int NAME                 = 1;
	public static final int ACL                  = 2;
	public static final int CUSTOM_ATTRIBUTES    = 3;
	public static final int AUTOBIND_FILTER      = 4;
	public static final int LINK_COLOR           = 5;
	public static final int POLICY_CONFIG        = 6;
	public static final int VERSION              = 7;
	public static final int DESCRIPTION          = 8;
	public static final int AGENT_PORT           = 9;
	public static final int AGENT_AUTH           = 10;
	public static final int SNMP_VERSION         = 11;
	public static final int SNMP_AUTH            = 12;
	public static final int AGENT_PROXY          = 13;
	public static final int SNMP_PROXY           = 14;
	public static final int TRUSTED_NODES        = 15;
	public static final int GEOLOCATION          = 16;
	public static final int PRIMARY_IP           = 17;
	public static final int SNMP_PORT            = 18;
	public static final int MAP_LAYOUT           = 19;
	public static final int MAP_BACKGROUND       = 20;
	public static final int MAP_CONTENT          = 21;
	public static final int IMAGE                = 22;
	public static final int ICMP_PROXY           = 23;
	public static final int COLUMN_COUNT         = 24;
	public static final int DASHBOARD_ELEMENTS   = 25;
	public static final int SCRIPT               = 26;
	public static final int ACTIVATION_EVENT     = 27;
	public static final int DEACTIVATION_EVENT   = 28;
	public static final int SOURCE_OBJECT        = 29;
	public static final int ACTIVE_STATUS        = 30;
	public static final int INACTIVE_STATUS      = 31;
	public static final int DCI_LIST             = 32;
	public static final int DRILL_DOWN_OBJECT_ID = 33;
	public static final int IP_ADDRESS           = 34;
	public static final int IP_PROTOCOL          = 35;
	public static final int IP_PORT              = 36;
	public static final int SERVICE_TYPE         = 37;
	public static final int POLLER_NODE          = 38;
	public static final int REQUIRED_POLLS       = 39;
	public static final int REQUEST              = 40;
	public static final int RESPONSE             = 41;
	public static final int OBJECT_FLAGS         = 42;
	public static final int IFXTABLE_POLICY      = 43;
	public static final int REPORT_DEFINITION    = 44;
	public static final int CLUSTER_RESOURCES    = 45;
	public static final int PRIMARY_NAME         = 46;
	public static final int STATUS_CALCULATION   = 47;
	public static final int CLUSTER_NETWORKS     = 48;
	public static final int EXPECTED_STATE       = 49;
	public static final int CONNECTION_ROUTING   = 50;
	public static final int DISCOVERY_RADIUS     = 51;
	public static final int HEIGHT               = 52;
	public static final int FILTER               = 53;
   public static final int PEER_GATEWAY         = 54;
   public static final int VPN_NETWORKS         = 55;
   public static final int POSTAL_ADDRESS       = 56;
   public static final int AGENT_CACHE_MODE     = 57;
   public static final int MAPOBJ_DISP_MODE     = 58;
   public static final int RACK_PLACEMENT       = 59;
   public static final int DASHBOARD_LIST       = 60;
   public static final int RACK_NUMB_SCHEME     = 61;
   public static final int CONTROLLER_ID        = 62;
   public static final int CHASSIS_ID           = 63;
   public static final int SSH_PROXY            = 64;
   public static final int SSH_LOGIN            = 65;
   public static final int SSH_PASSWORD         = 66;
   public static final int ZONE_PROXY           = 67;
	
	private Set<Integer> fieldSet;
	private long objectId;
	private String name;
	private String primaryName;
	private AccessListElement[] acl;
	private boolean inheritAccessRights;
	private Map<String, String> customAttributes;
	private String autoBindFilter;
	private String configFileContent;
	private int version;
	private String description;
	private int agentPort;
	private int agentAuthMethod;
	private String agentSecret;
	private long agentProxy;
	private int snmpPort;
	private int snmpVersion;
	private int snmpAuthMethod;
	private int snmpPrivMethod;
	private String snmpAuthName;
	private String snmpAuthPassword;
	private String snmpPrivPassword;
	private long snmpProxy;
	private long icmpProxy;
	private long[] trustedNodes;
	private GeoLocation geolocation;
	private InetAddress primaryIpAddress;
	private MapLayoutAlgorithm mapLayout;
	private UUID mapBackground;
	private GeoLocation mapBackgroundLocation;
	private int mapBackgroundZoom;
	private int mapBackgroundColor;
	private UUID image;
	private Collection<NetworkMapElement> mapElements;
	private Collection<NetworkMapLink> mapLinks;
	private int columnCount;
	private Collection<DashboardElement> dashboardElements;
	private String script;
	private int activationEvent;
	private int deactivationEvent;
	private long sourceObject;
	private int activeStatus;
	private int inactiveStatus;
	private List<ConditionDciInfo> dciList;
	private long drillDownObjectId;
	private long pollerNode;
	private int requiredPolls;
	private int serviceType;
	private int ipProtocol;
	private int ipPort;
	private InetAddressEx ipAddress;
	private String request;
	private String response;
	private int objectFlags;
   private int objectFlagsMask;
	private int ifXTablePolicy;
	private String reportDefinition;
	private List<ClusterResource> resourceList;
	private List<InetAddressEx> networkList;
	private int statusCalculationMethod;
	private int statusPropagationMethod;
	private ObjectStatus fixedPropagatedStatus;
	private int statusShift;
	private ObjectStatus[] statusTransformation;
	private int statusSingleThreshold;
	private int[] statusThresholds;
	private int expectedState;
	private int linkColor;
	private int connectionRouting;
	private int discoveryRadius;
	private int height;
	private String filter;
	private long peerGatewayId;
	private List<InetAddressEx> localNetworks;
	private List<InetAddressEx> remoteNetworks;
	private PostalAddress postalAddress;
	private AgentCacheMode agentCacheMode;
	private MapObjectDisplayMode mapObjectDisplayMode;
	private long rackId;
	private UUID rackImage;
	private short rackPosition;
	private short rackHeight;
	private Long[] dashboards;
	private boolean rackNumberingTopBottom;
	private long controllerId;
	private long chassisId;
	private long sshProxy;
	private String sshLogin;
	private String sshPassword;
	private long zoneProxy;
	
	/**
	 * Constructor for creating modification data for given object
	 */
	public NXCObjectModificationData(long objectId)
	{
		this.objectId = objectId;
		fieldSet = new HashSet<Integer>(128);
	}

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setObjectId(long objectId)
	{
		this.objectId = objectId;
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
	public void setName(final String name)
	{
		this.name = name;
		fieldSet.add(NAME);
	}

	/**
	 * Check if given field is set for modification.
	 * 
	 * @param field field code
	 * @return true if given field is set
	 */
	public boolean isFieldSet(int field)
	{
		return fieldSet.contains(field);
	}

	/**
	 * @return the acl
	 */
	public AccessListElement[] getACL()
	{
		return (acl != null) ? acl : new AccessListElement[0];
	}

	/**
	 * @param acl the acl to set
	 */
	public void setACL(AccessListElement[] acl)
	{
		this.acl = acl;
		fieldSet.add(ACL);
	}

	/**
	 * @return the inheritAccessRights
	 */
	public boolean isInheritAccessRights()
	{
		return inheritAccessRights;
	}

	/**
	 * @param inheritAccessRights the inheritAccessRights to set
	 */
	public void setInheritAccessRights(boolean inheritAccessRights)
	{
		this.inheritAccessRights = inheritAccessRights;
		fieldSet.add(ACL);
	}

	/**
	 * @return the customAttributes
	 */
	public Map<String, String> getCustomAttributes()
	{
		return customAttributes;
	}

	/**
	 * @param customAttributes the customAttributes to set
	 */
	public void setCustomAttributes(Map<String, String> customAttributes)
	{
		this.customAttributes = customAttributes;
		fieldSet.add(CUSTOM_ATTRIBUTES);
	}

	/**
	 * @return the autoApplyFilter
	 */
	public String getAutoBindFilter()
	{
		return autoBindFilter;
	}

	/**
	 * @param autoApplyFilter the autoApplyFilter to set
	 */
	public void setAutoBindFilter(String autoBindFilter)
	{
		this.autoBindFilter = autoBindFilter;
		fieldSet.add(AUTOBIND_FILTER);
	}

	/**
	 * @return the configFileContent
	 */
	public String getConfigFileContent()
	{
		return configFileContent;
	}

	/**
	 * @param configFileContent the configFileContent to set
	 */
	public void setConfigFileContent(String configFileContent)
	{
		this.configFileContent = configFileContent;
		fieldSet.add(POLICY_CONFIG);
	}

	/**
	 * @return the version
	 */
	public int getVersion()
	{
		return version;
	}

	/**
	 * @param version the version to set
	 */
	public void setVersion(int version)
	{
		this.version = version;
		fieldSet.add(VERSION);
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
		fieldSet.add(DESCRIPTION);
	}

	public int getAgentPort()
	{
		return agentPort;
	}

	public void setAgentPort(int agentPort)
	{
		this.agentPort = agentPort;
		fieldSet.add(AGENT_PORT);
	}

	/**
	 * @return the agentAuthMethod
	 */
	public int getAgentAuthMethod()
	{
		return agentAuthMethod;
	}

	/**
	 * @param agentAuthMethod the agentAuthMethod to set
	 */
	public void setAgentAuthMethod(int agentAuthMethod)
	{
		this.agentAuthMethod = agentAuthMethod;
		fieldSet.add(AGENT_AUTH);
	}

	/**
	 * @return the agentSecret
	 */
	public String getAgentSecret()
	{
		return agentSecret;
	}

	/**
	 * @param agentSecret the agentSecret to set
	 */
	public void setAgentSecret(String agentSecret)
	{
		this.agentSecret = agentSecret;
		fieldSet.add(AGENT_AUTH);
	}

	/**
	 * @return the agentProxy
	 */
	public long getAgentProxy()
	{
		return agentProxy;
	}

	/**
	 * @param agentProxy the agentProxy to set
	 */
	public void setAgentProxy(long agentProxy)
	{
		this.agentProxy = agentProxy;
		fieldSet.add(AGENT_PROXY);
	}

	/**
	 * @return the snmpVersion
	 */
	public int getSnmpVersion()
	{
		return snmpVersion;
	}

	/**
	 * @param snmpVersion the snmpVersion to set
	 */
	public void setSnmpVersion(int snmpVersion)
	{
		this.snmpVersion = snmpVersion;
		fieldSet.add(SNMP_VERSION);
	}

	/**
	 * @return the snmpAuthMethod
	 */
	public int getSnmpAuthMethod()
	{
		return snmpAuthMethod;
	}

	/**
	 * @param snmpAuthMethod the snmpAuthMethod to set
	 */
	public void setSnmpAuthMethod(int snmpAuthMethod)
	{
		this.snmpAuthMethod = snmpAuthMethod;
		fieldSet.add(SNMP_AUTH);
	}

	/**
	 * @return the snmpPrivMethod
	 */
	public int getSnmpPrivMethod()
	{
		return snmpPrivMethod;
	}

	/**
	 * @param snmpPrivMethod the snmpPrivMethod to set
	 */
	public void setSnmpPrivMethod(int snmpPrivMethod)
	{
		this.snmpPrivMethod = snmpPrivMethod;
		fieldSet.add(SNMP_AUTH);
	}

	/**
	 * @return the snmpAuthName
	 */
	public String getSnmpAuthName()
	{
		return snmpAuthName;
	}

	/**
	 * @param snmpAuthName the snmpAuthName to set
	 */
	public void setSnmpAuthName(String snmpAuthName)
	{
		this.snmpAuthName = snmpAuthName;
		fieldSet.add(SNMP_AUTH);
	}

	/**
	 * @return the snmpAuthPassword
	 */
	public String getSnmpAuthPassword()
	{
		return snmpAuthPassword;
	}

	/**
	 * @param snmpAuthPassword the snmpAuthPassword to set
	 */
	public void setSnmpAuthPassword(String snmpAuthPassword)
	{
		this.snmpAuthPassword = snmpAuthPassword;
		fieldSet.add(SNMP_AUTH);
	}

	/**
	 * @return the snmpPrivPassword
	 */
	public String getSnmpPrivPassword()
	{
		return snmpPrivPassword;
	}

	/**
	 * @param snmpPrivPassword the snmpPrivPassword to set
	 */
	public void setSnmpPrivPassword(String snmpPrivPassword)
	{
		this.snmpPrivPassword = snmpPrivPassword;
		fieldSet.add(SNMP_AUTH);
	}

	/**
	 * @return the snmpProxy
	 */
	public long getSnmpProxy()
	{
		return snmpProxy;
	}

	/**
	 * @param snmpProxy the snmpProxy to set
	 */
	public void setSnmpProxy(long snmpProxy)
	{
		this.snmpProxy = snmpProxy;
		fieldSet.add(SNMP_PROXY);
	}

	/**
	 * @return the icmpProxy
	 */
	public long getIcmpProxy()
	{
		return icmpProxy;
	}

	/**
	 * @param icmpProxy the icmpProxy to set
	 */
	public void setIcmpProxy(long icmpProxy)
	{
		this.icmpProxy = icmpProxy;
		fieldSet.add(ICMP_PROXY);
	}

	/**
	 * @return the trustedNodes
	 */
	public long[] getTrustedNodes()
	{
		return trustedNodes;
	}

	/**
	 * @param trustedNodes the trustedNodes to set
	 */
	public void setTrustedNodes(long[] trustedNodes)
	{
		this.trustedNodes = trustedNodes;
		fieldSet.add(TRUSTED_NODES);
	}

	/**
	 * @return the geolocation
	 */
	public GeoLocation getGeolocation()
	{
		return geolocation;
	}

	/**
	 * @param geolocation the geolocation to set
	 */
	public void setGeolocation(GeoLocation geolocation)
	{
		this.geolocation = geolocation;
		fieldSet.add(GEOLOCATION);
	}

	/**
	 * @return the primaryIpAddress
	 */
	public InetAddress getPrimaryIpAddress()
	{
		return primaryIpAddress;
	}

	/**
	 * @param primaryIpAddress the primaryIpAddress to set
	 */
	public void setPrimaryIpAddress(InetAddress primaryIpAddress)
	{
		this.primaryIpAddress = primaryIpAddress;
		fieldSet.add(PRIMARY_IP);
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
		fieldSet.add(SNMP_PORT);
	}

	/**
	 * @return the mapLayout
	 */
	public MapLayoutAlgorithm getMapLayout()
	{
		return mapLayout;
	}

	/**
	 * @param mapLayout the mapLayout to set
	 */
	public void setMapLayout(MapLayoutAlgorithm mapLayout)
	{
		this.mapLayout = mapLayout;
		fieldSet.add(MAP_LAYOUT);
	}

	/**
	 * @return the mapBackground
	 */
	public UUID getMapBackground()
	{
		return mapBackground;
	}

	/**
	 * @param mapBackground the mapBackground to set
	 */
	public void setMapBackground(UUID mapBackground, GeoLocation mapBackgroundLocation, int mapBackgroundZoom, int mapBackgroundColor)
	{
		this.mapBackground = mapBackground;
		this.mapBackgroundLocation = mapBackgroundLocation;
		this.mapBackgroundZoom = mapBackgroundZoom;
		this.mapBackgroundColor = mapBackgroundColor;
		fieldSet.add(MAP_BACKGROUND);
	}

	/**
	 * @return the mapElements
	 */
	public Collection<NetworkMapElement> getMapElements()
	{
		return mapElements;
	}

	/**
	 * @return the mapLinks
	 */
	public Collection<NetworkMapLink> getMapLinks()
	{
		return mapLinks;
	}
	
	/**
	 * Set map contents
	 * 
	 * @param elements
	 * @param links
	 */
	public void setMapContent(Collection<NetworkMapElement> elements, Collection<NetworkMapLink> links)
	{
		mapElements = elements;
		mapLinks = links;
		fieldSet.add(MAP_CONTENT);
	}

	/**
	 * @return the image
	 */
	public UUID getImage()
	{
		return image;
	}

	/**
	 * @param image the image to set
	 */
	public void setImage(UUID image)
	{
		this.image = image;
		fieldSet.add(IMAGE);
	}

	/**
	 * @return the columnCount
	 */
	public int getColumnCount()
	{
		return columnCount;
	}

	/**
	 * @param columnCount the columnCount to set
	 */
	public void setColumnCount(int columnCount)
	{
		this.columnCount = columnCount;
		fieldSet.add(COLUMN_COUNT);
	}

	/**
	 * @return the dashboardElements
	 */
	public Collection<DashboardElement> getDashboardElements()
	{
		return dashboardElements;
	}

	/**
	 * @param dashboardElements the dashboardElements to set
	 */
	public void setDashboardElements(Collection<DashboardElement> dashboardElements)
	{
		this.dashboardElements = dashboardElements;
		fieldSet.add(DASHBOARD_ELEMENTS);
	}

	/**
	 * @return the script
	 */
	public String getScript()
	{
		return script;
	}

	/**
	 * @param script the script to set
	 */
	public void setScript(String script)
	{
		this.script = script;
		fieldSet.add(SCRIPT);
	}

	/**
	 * @return the activationEvent
	 */
	public int getActivationEvent()
	{
		return activationEvent;
	}

	/**
	 * @param activationEvent the activationEvent to set
	 */
	public void setActivationEvent(int activationEvent)
	{
		this.activationEvent = activationEvent;
		fieldSet.add(ACTIVATION_EVENT);
	}

	/**
	 * @return the deactivationEvent
	 */
	public int getDeactivationEvent()
	{
		return deactivationEvent;
	}

	/**
	 * @param deactivationEvent the deactivationEvent to set
	 */
	public void setDeactivationEvent(int deactivationEvent)
	{
		this.deactivationEvent = deactivationEvent;
		fieldSet.add(DEACTIVATION_EVENT);
	}

	/**
	 * @return the sourceObject
	 */
	public long getSourceObject()
	{
		return sourceObject;
	}

	/**
	 * @param sourceObject the sourceObject to set
	 */
	public void setSourceObject(long sourceObject)
	{
		this.sourceObject = sourceObject;
		fieldSet.add(SOURCE_OBJECT);
	}

	/**
	 * @return the activeStatus
	 */
	public int getActiveStatus()
	{
		return activeStatus;
	}

	/**
	 * @param activeStatus the activeStatus to set
	 */
	public void setActiveStatus(int activeStatus)
	{
		this.activeStatus = activeStatus;
		fieldSet.add(ACTIVE_STATUS);
	}

	/**
	 * @return the inactiveStatus
	 */
	public int getInactiveStatus()
	{
		return inactiveStatus;
	}

	/**
	 * @param inactiveStatus the inactiveStatus to set
	 */
	public void setInactiveStatus(int inactiveStatus)
	{
		this.inactiveStatus = inactiveStatus;
		fieldSet.add(INACTIVE_STATUS);
	}

	/**
	 * @return the dciList
	 */
	public List<ConditionDciInfo> getDciList()
	{
		return dciList;
	}

	/**
	 * @param dciList the dciList to set
	 */
	public void setDciList(List<ConditionDciInfo> dciList)
	{
		this.dciList = dciList;
		fieldSet.add(DCI_LIST);
	}

	/**
	 * @return the submapId
	 */
	public long getDrillDownObjectId()
	{
		return drillDownObjectId;
	}

	/**
	 * @param drillDownObjectId the drillDownObjectId to set
	 */
	public void setDrillDownObjectId(long drillDownObjectId)
	{
		this.drillDownObjectId = drillDownObjectId;
		fieldSet.add(DRILL_DOWN_OBJECT_ID);
	}

	/**
	 * @return the mapBackgroundLocation
	 */
	public GeoLocation getMapBackgroundLocation()
	{
		return mapBackgroundLocation;
	}

	/**
	 * @return the mapBackgroundZoom
	 */
	public int getMapBackgroundZoom()
	{
		return mapBackgroundZoom;
	}

	/**
	 * @return the pollerNode
	 */
	public long getPollerNode()
	{
		return pollerNode;
	}

	/**
	 * @param pollerNode the pollerNode to set
	 */
	public void setPollerNode(long pollerNode)
	{
		this.pollerNode = pollerNode;
		fieldSet.add(POLLER_NODE);
	}

	/**
	 * @return the requiredPolls
	 */
	public int getRequiredPolls()
	{
		return requiredPolls;
	}

	/**
	 * @param requiredPolls the requiredPolls to set
	 */
	public void setRequiredPolls(int requiredPolls)
	{
		this.requiredPolls = requiredPolls;
		fieldSet.add(REQUIRED_POLLS);
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
		fieldSet.add(SERVICE_TYPE);
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
		fieldSet.add(IP_PROTOCOL);
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
		fieldSet.add(IP_PORT);
	}

	/**
	 * @return the ipAddress
	 */
	public InetAddressEx getIpAddress()
	{
		return ipAddress;
	}

	/**
	 * @param ipAddress the ipAddress to set
	 */
	public void setIpAddress(InetAddressEx ipAddress)
	{
		this.ipAddress = ipAddress;
		fieldSet.add(IP_ADDRESS);
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
		fieldSet.add(REQUEST);
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
		fieldSet.add(RESPONSE);
	}

	/**
	 * @return the nodeFlags
	 */
	public int getObjectFlags()
	{
		return objectFlags;
	}

   /**
    * @return the objectFlagsMask
    */
   public int getObjectFlagsMask()
   {
      return objectFlagsMask;
   }

	/**
	 * @param nodeFlags the nodeFlags to set
	 */
	public void setObjectFlags(int objectFlags)
	{
	   setObjectFlags(objectFlags, 0x7FFFFFFF);
	}

   /**
    * @param nodeFlags the nodeFlags to set
    */
   public void setObjectFlags(int objectFlags, int objectFlagsMask)
   {
      this.objectFlags = objectFlags;
      this.objectFlagsMask = objectFlagsMask;
      fieldSet.add(OBJECT_FLAGS);
   }

   /**
	 * @return the ifXTablePolicy
	 */
	public int getIfXTablePolicy()
	{
		return ifXTablePolicy;
	}

	/**
	 * @param ifXTablePolicy the ifXTablePolicy to set
	 */
	public void setIfXTablePolicy(int ifXTablePolicy)
	{
		this.ifXTablePolicy = ifXTablePolicy;
		fieldSet.add(IFXTABLE_POLICY);
	}

	/**
	 * @return the reportDefinition
	 */
	public String getReportDefinition()
	{
		return reportDefinition;
	}

	/**
	 * @param reportDefinition the reportDefinition to set
	 */
	public void setReportDefinition(String reportDefinition)
	{
		this.reportDefinition = reportDefinition;
		fieldSet.add(REPORT_DEFINITION);
	}

	/**
	 * Set report definition from file.
	 * 
	 * @param file file containing report definition
	 * @throws IOException if file I/O error occurs
	 * @throws FileNotFoundException if given file does not exist or inaccessible
	 */
	public void setReportDefinition(File file) throws IOException, FileNotFoundException
	{
		byte[] buffer = new byte[(int)file.length()];
		FileInputStream in = new FileInputStream(file);
		try
		{
			in.read(buffer);
		}
		finally
		{
			if (in != null)
				in.close();
		}
		setReportDefinition(new String(buffer));
	}

	/**
	 * @return the resourceList
	 */
	public List<ClusterResource> getResourceList()
	{
		return resourceList;
	}

	/**
	 * @param resourceList the resourceList to set
	 */
	public void setResourceList(List<ClusterResource> resourceList)
	{
		this.resourceList = resourceList;
		fieldSet.add(CLUSTER_RESOURCES);
	}

	/**
	 * @return the networkList
	 */
	public List<InetAddressEx> getNetworkList()
	{
		return networkList;
	}

	/**
	 * @param networkList the networkList to set
	 */
	public void setNetworkList(List<InetAddressEx> networkList)
	{
		this.networkList = networkList;
		fieldSet.add(CLUSTER_NETWORKS);
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
		fieldSet.add(PRIMARY_NAME);
	}

	/**
	 * @return the statusCalculationMethod
	 */
	public int getStatusCalculationMethod()
	{
		return statusCalculationMethod;
	}

	/**
	 * @param statusCalculationMethod the statusCalculationMethod to set
	 */
	public void setStatusCalculationMethod(int statusCalculationMethod)
	{
		this.statusCalculationMethod = statusCalculationMethod;
		fieldSet.add(STATUS_CALCULATION);
	}

	/**
	 * @return the statusPropagationMethod
	 */
	public int getStatusPropagationMethod()
	{
		return statusPropagationMethod;
	}

	/**
	 * @param statusPropagationMethod the statusPropagationMethod to set
	 */
	public void setStatusPropagationMethod(int statusPropagationMethod)
	{
		this.statusPropagationMethod = statusPropagationMethod;
		fieldSet.add(STATUS_CALCULATION);
	}

	/**
	 * @return the fixedPropagatedStatus
	 */
	public ObjectStatus getFixedPropagatedStatus()
	{
		return fixedPropagatedStatus;
	}

	/**
	 * @param fixedPropagatedStatus the fixedPropagatedStatus to set
	 */
	public void setFixedPropagatedStatus(ObjectStatus fixedPropagatedStatus)
	{
		this.fixedPropagatedStatus = fixedPropagatedStatus;
		fieldSet.add(STATUS_CALCULATION);
	}

	/**
	 * @return the statusShift
	 */
	public int getStatusShift()
	{
		return statusShift;
	}

	/**
	 * @param statusShift the statusShift to set
	 */
	public void setStatusShift(int statusShift)
	{
		this.statusShift = statusShift;
		fieldSet.add(STATUS_CALCULATION);
	}

	/**
	 * @return the statusTransformation
	 */
	public ObjectStatus[] getStatusTransformation()
	{
		return statusTransformation;
	}

	/**
	 * @param statusTransformation the statusTransformation to set
	 */
	public void setStatusTransformation(ObjectStatus[] statusTransformation)
	{
		this.statusTransformation = statusTransformation;
		fieldSet.add(STATUS_CALCULATION);
	}

	/**
	 * @return the statusSingleThreshold
	 */
	public int getStatusSingleThreshold()
	{
		return statusSingleThreshold;
	}

	/**
	 * @param statusSingleThreshold the statusSingleThreshold to set
	 */
	public void setStatusSingleThreshold(int statusSingleThreshold)
	{
		this.statusSingleThreshold = statusSingleThreshold;
		fieldSet.add(STATUS_CALCULATION);
	}

	/**
	 * @return the statusThresholds
	 */
	public int[] getStatusThresholds()
	{
		return statusThresholds;
	}

	/**
	 * @param statusThresholds the statusThresholds to set
	 */
	public void setStatusThresholds(int[] statusThresholds)
	{
		this.statusThresholds = statusThresholds;
		fieldSet.add(STATUS_CALCULATION);
	}

	/**
	 * @return the expectedState
	 */
	public int getExpectedState()
	{
		return expectedState;
	}

	/**
	 * @param expectedState the expectedState to set
	 */
	public void setExpectedState(int expectedState)
	{
		this.expectedState = expectedState;
		fieldSet.add(EXPECTED_STATE);
	}

	/**
	 * @return the linkColor
	 */
	public int getLinkColor()
	{
		return linkColor;
	}

	/**
	 * @param linkColor the linkColor to set
	 */
	public void setLinkColor(int linkColor)
	{
		this.linkColor = linkColor;
		fieldSet.add(LINK_COLOR);
	}

	/**
	 * @return the connectionRouting
	 */
	public int getConnectionRouting()
	{
		return connectionRouting;
	}

	/**
	 * @param connectionRouting the connectionRouting to set
	 */
	public void setConnectionRouting(int connectionRouting)
	{
		this.connectionRouting = connectionRouting;
		fieldSet.add(CONNECTION_ROUTING);
	}

	/**
	 * @return the mapBackgroundColor
	 */
	public int getMapBackgroundColor()
	{
		return mapBackgroundColor;
	}

	/**
	 * @return the discoveryRadius
	 */
	public final int getDiscoveryRadius()
	{
		return discoveryRadius;
	}

	/**
	 * @param discoveryRadius the discoveryRadius to set
	 */
	public final void setDiscoveryRadius(int discoveryRadius)
	{
		this.discoveryRadius = discoveryRadius;
		fieldSet.add(DISCOVERY_RADIUS);
	}

	/**
	 * @return the height
	 */
	public int getHeight()
	{
		return height;
	}

	/**
	 * @param height the height to set
	 */
	public void setHeight(int height)
	{
		this.height = height;
		fieldSet.add(HEIGHT);
	}

	/**
	 * @return the filter
	 */
	public String getFilter()
	{
		return filter;
	}

	/**
	 * @param filter the filter to set
	 */
	public void setFilter(String filter)
	{
		this.filter = filter;
		fieldSet.add(FILTER);
	}

   /**
    * @return the peerGatewayId
    */
   public long getPeerGatewayId()
   {
      return peerGatewayId;
   }

   /**
    * @param peerGatewayId the peerGatewayId to set
    */
   public void setPeerGatewayId(long peerGatewayId)
   {
      this.peerGatewayId = peerGatewayId;
      fieldSet.add(PEER_GATEWAY);
   }

   /**
    * @return the localNetworks
    */
   public List<InetAddressEx> getLocalNetworks()
   {
      return localNetworks;
   }

   /**
    * @return the remoteNetworks
    */
   public List<InetAddressEx> getRemoteNetworks()
   {
      return remoteNetworks;
   }

   /**
    * @param remoteNetworks the remoteNetworks to set
    */
   public void setVpnNetworks(List<InetAddressEx> localNetworks, List<InetAddressEx> remoteNetworks)
   {
      this.localNetworks = localNetworks;
      this.remoteNetworks = remoteNetworks;
      fieldSet.add(VPN_NETWORKS);
   }

   /**
    * @return the postalAddress
    */
   public PostalAddress getPostalAddress()
   {
      return postalAddress;
   }

   /**
    * @param postalAddress the postalAddress to set
    */
   public void setPostalAddress(PostalAddress postalAddress)
   {
      this.postalAddress = postalAddress;
      fieldSet.add(POSTAL_ADDRESS);
   }

   /**
    * @return the agentCacheMode
    */
   public AgentCacheMode getAgentCacheMode()
   {
      return agentCacheMode;
   }

   /**
    * @param agentCacheMode the agentCacheMode to set
    */
   public void setAgentCacheMode(AgentCacheMode agentCacheMode)
   {
      this.agentCacheMode = agentCacheMode;
      fieldSet.add(AGENT_CACHE_MODE);
   }

   /**
    * @return the mapObjectDisplayMode
    */
   public MapObjectDisplayMode getMapObjectDisplayMode()
   {
      return mapObjectDisplayMode;
   }

   /**
    * @param mapObjectDisplayMode the mapObjectDisplayMode to set
    */
   public void setMapObjectDisplayMode(MapObjectDisplayMode mapObjectDisplayMode)
   {
      this.mapObjectDisplayMode = mapObjectDisplayMode;
      fieldSet.add(MAPOBJ_DISP_MODE);
   }

   /**
    * @return the rackId
    */
   public long getRackId()
   {
      return rackId;
   }

   /**
    * @return the rackImage
    */
   public UUID getRackImage()
   {
      return rackImage;
   }

   /**
    * @return the rackPosition
    */
   public short getRackPosition()
   {
      return rackPosition;
   }

   /**
    * @return the rackHeight
    */
   public short getRackHeight()
   {
      return rackHeight;
   }
   
   /**
    * Set rack placement data
    * 
    * @param rackId
    * @param rackImage
    * @param rackPosition
    * @param rackHeight
    */
   public void setRackPlacement(long rackId, UUID rackImage, short rackPosition, short rackHeight)
   {
      this.rackId = rackId;
      this.rackImage = rackImage;
      this.rackPosition = rackPosition;
      this.rackHeight = rackHeight;
      fieldSet.add(RACK_PLACEMENT);
   }

   /**
    * @return the dashboards
    */
   public Long[] getDashboards()
   {
      return dashboards;
   }

   /**
    * @param dashboards the dashboards to set
    */
   public void setDashboards(Long[] dashboards)
   {
      this.dashboards = dashboards;
      fieldSet.add(DASHBOARD_LIST);
   }

   /**
    * @return the rackNumberingTopBottom
    */
   public boolean isRackNumberingTopBottom()
   {
      return rackNumberingTopBottom;
   }

   /**
    * @param rackNumberingTopBottom the rackNumberingTopBottom to set
    */
   public void setRackNumberingTopBottom(boolean rackNumberingTopBottom)
   {
      this.rackNumberingTopBottom = rackNumberingTopBottom;
      fieldSet.add(RACK_NUMB_SCHEME);
   }

   /**
    * @return the controllerId
    */
   public long getControllerId()
   {
      return controllerId;
   }

   /**
    * @param controllerId the controllerId to set
    */
   public void setControllerId(long controllerId)
   {
      this.controllerId = controllerId;
      fieldSet.add(CONTROLLER_ID);
   }

   /**
    * @return the chassisId
    */
   public long getChassisId()
   {
      return chassisId;
   }

   /**
    * @param chassisId the chassisId to set
    */
   public void setChassisId(long chassisId)
   {
      this.chassisId = chassisId;
      fieldSet.add(CHASSIS_ID);
   }

   /**
    * @return the sshProxy
    */
   public long getSshProxy()
   {
      return sshProxy;
   }

   /**
    * @param sshProxy the sshProxy to set
    */
   public void setSshProxy(long sshProxy)
   {
      this.sshProxy = sshProxy;
      fieldSet.add(SSH_PROXY);
   }

   /**
    * @return the sshLogin
    */
   public String getSshLogin()
   {
      return sshLogin;
   }

   /**
    * @param sshLogin the sshLogin to set
    */
   public void setSshLogin(String sshLogin)
   {
      this.sshLogin = sshLogin;
      fieldSet.add(SSH_LOGIN);
   }

   /**
    * @return the sshPassword
    */
   public String getSshPassword()
   {
      return sshPassword;
   }

   /**
    * @param sshPassword the sshPassword to set
    */
   public void setSshPassword(String sshPassword)
   {
      this.sshPassword = sshPassword;
      fieldSet.add(SSH_PASSWORD);
   }

   /**
    * @return the zoneProxy
    */
   public long getZoneProxy()
   {
      return zoneProxy;
   }

   /**
    * @param zoneProxy the zoneProxy to set
    */
   public void setZoneProxy(long zoneProxy)
   {
      this.zoneProxy = zoneProxy;
      fieldSet.add(ZONE_PROXY);
   }
}
