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
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.GeoLocation;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ConditionDciInfo;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.objects.ClusterResource;
import org.netxms.client.objects.ClusterSyncNetwork;

/**
 * This class is used to hold data for NXCSession.modifyObject()
 */
public class NXCObjectModificationData
{
	// Modification flags
	public static final long MODIFY_NAME               = 0x00000000000001L;
	public static final long MODIFY_ACL                = 0x00000000000002L;
	public static final long MODIFY_CUSTOM_ATTRIBUTES  = 0x00000000000004L;
	public static final long MODIFY_AUTOBIND_FILTER    = 0x00000000000008L;
	public static final long MODIFY_LINK_COLOR         = 0x00000000000010L;
	public static final long MODIFY_POLICY_CONFIG      = 0x00000000000020L;
	public static final long MODIFY_VERSION            = 0x00000000000040L;
	public static final long MODIFY_DESCRIPTION        = 0x00000000000080L;
	public static final long MODIFY_AGENT_PORT         = 0x00000000000100L;
	public static final long MODIFY_AGENT_AUTH         = 0x00000000000200L;
	public static final long MODIFY_SNMP_VERSION       = 0x00000000000400L;
	public static final long MODIFY_SNMP_AUTH          = 0x00000000000800L;
	public static final long MODIFY_AGENT_PROXY        = 0x00000000001000L;
	public static final long MODIFY_SNMP_PROXY         = 0x00000000002000L;
	public static final long MODIFY_TRUSTED_NODES      = 0x00000000004000L;
	public static final long MODIFY_GEOLOCATION        = 0x00000000008000L;
	public static final long MODIFY_PRIMARY_IP         = 0x00000000010000L;
	public static final long MODIFY_SNMP_PORT          = 0x00000000020000L;
	public static final long MODIFY_MAP_LAYOUT         = 0x00000000040000L;
	public static final long MODIFY_MAP_BACKGROUND     = 0x00000000080000L;
	public static final long MODIFY_MAP_CONTENT        = 0x00000000100000L;
	public static final long MODIFY_IMAGE              = 0x00000000200000L;
	public static final long MODIFY_ICMP_PROXY         = 0x00000000400000L;
	public static final long MODIFY_COLUMN_COUNT       = 0x00000000800000L;
	public static final long MODIFY_DASHBOARD_ELEMENTS = 0x00000001000000L;
	public static final long MODIFY_SCRIPT             = 0x00000002000000L;
	public static final long MODIFY_ACTIVATION_EVENT   = 0x00000004000000L;
	public static final long MODIFY_DEACTIVATION_EVENT = 0x00000008000000L;
	public static final long MODIFY_SOURCE_OBJECT      = 0x00000010000000L;
	public static final long MODIFY_ACTIVE_STATUS      = 0x00000020000000L;
	public static final long MODIFY_INACTIVE_STATUS    = 0x00000040000000L;
	public static final long MODIFY_DCI_LIST           = 0x00000080000000L;
	public static final long MODIFY_SUBMAP_ID          = 0x00000100000000L;
	public static final long MODIFY_IP_ADDRESS         = 0x00000200000000L;
	public static final long MODIFY_IP_PROTOCOL        = 0x00000400000000L;
	public static final long MODIFY_IP_PORT            = 0x00000800000000L;
	public static final long MODIFY_SERVICE_TYPE       = 0x00001000000000L;
	public static final long MODIFY_POLLER_NODE        = 0x00002000000000L;
	public static final long MODIFY_REQUIRED_POLLS     = 0x00004000000000L;
	public static final long MODIFY_REQUEST            = 0x00008000000000L;
	public static final long MODIFY_RESPONSE           = 0x00010000000000L;
	public static final long MODIFY_OBJECT_FLAGS       = 0x00020000000000L;
	public static final long MODIFY_IFXTABLE_POLICY    = 0x00040000000000L;
	public static final long MODIFY_REPORT_DEFINITION  = 0x00080000000000L;
	public static final long MODIFY_CLUSTER_RESOURCES  = 0x00100000000000L;
	public static final long MODIFY_PRIMARY_NAME       = 0x00200000000000L;
	public static final long MODIFY_STATUS_CALCULATION = 0x00400000000000L;
	public static final long MODIFY_CLUSTER_NETWORKS   = 0x00800000000000L;
	public static final long MODIFY_EXPECTED_STATE     = 0x01000000000000L;
	public static final long MODIFY_CONNECTION_ROUTING = 0x02000000000000L;
	public static final long MODIFY_DISCOVERY_RADIUS   = 0x04000000000000L;
	public static final long MODIFY_HEIGHT             = 0x08000000000000L;
	public static final long MODIFY_FILTER             = 0x10000000000000L;
	
	private long flags;		// Flags which indicates what object's data should be modified
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
	private int mapLayout;
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
	private long submapId;
	private long pollerNode;
	private int requiredPolls;
	private int serviceType;
	private int ipProtocol;
	private int ipPort;
	private int ipAddress;
	private String request;
	private String response;
	private int objectFlags;
	private int ifXTablePolicy;
	private String reportDefinition;
	private List<ClusterResource> resourceList;
	private List<ClusterSyncNetwork> networkList;
	private int statusCalculationMethod;
	private int statusPropagationMethod;
	private int fixedPropagatedStatus;
	private int statusShift;
	private int[] statusTransformation;
	private int statusSingleThreshold;
	private int[] statusThresholds;
	private int expectedState;
	private int linkColor;
	private int connectionRouting;
	private int discoveryRadius;
	private int height;
	private String filter;
	
	/**
	 * Constructor for creating modification data for given object
	 */
	public NXCObjectModificationData(long objectId)
	{
		this.objectId = objectId;
		flags = 0;
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
		flags |= MODIFY_NAME;
	}

	/**
	 * @return the flags
	 */
	public long getFlags()
	{
		return flags;
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
		flags |= MODIFY_ACL;
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
		flags |= MODIFY_ACL;
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
		flags |= MODIFY_CUSTOM_ATTRIBUTES;
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
		flags |= MODIFY_AUTOBIND_FILTER;
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
		flags |= MODIFY_POLICY_CONFIG;
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
		flags |= MODIFY_VERSION;
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
		flags |= MODIFY_DESCRIPTION;
	}

	public int getAgentPort()
	{
		return agentPort;
	}

	public void setAgentPort(int agentPort)
	{
		this.agentPort = agentPort;
		flags |= MODIFY_AGENT_PORT;
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
		flags |= MODIFY_AGENT_AUTH;
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
		flags |= MODIFY_AGENT_AUTH;
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
		flags |= MODIFY_AGENT_PROXY;
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
		flags |= MODIFY_SNMP_VERSION;
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
		flags |= MODIFY_SNMP_AUTH;
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
		flags |= MODIFY_SNMP_AUTH;
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
		flags |= MODIFY_SNMP_AUTH;
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
		flags |= MODIFY_SNMP_AUTH;
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
		flags |= MODIFY_SNMP_AUTH;
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
		flags |= MODIFY_SNMP_PROXY;
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
		flags |= MODIFY_ICMP_PROXY;
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
		flags |= MODIFY_TRUSTED_NODES;
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
		flags |= MODIFY_GEOLOCATION;
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
		flags |= MODIFY_PRIMARY_IP;
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
		flags |= MODIFY_SNMP_PORT;
	}

	/**
	 * @return the mapLayout
	 */
	public int getMapLayout()
	{
		return mapLayout;
	}

	/**
	 * @param mapLayout the mapLayout to set
	 */
	public void setMapLayout(int mapLayout)
	{
		this.mapLayout = mapLayout;
		flags |= MODIFY_MAP_LAYOUT;
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
		flags |= MODIFY_MAP_BACKGROUND;
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
		flags |= MODIFY_MAP_CONTENT;
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
		flags |= MODIFY_IMAGE;
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
		flags |= MODIFY_COLUMN_COUNT;
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
		flags |= MODIFY_DASHBOARD_ELEMENTS;
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
		flags |= MODIFY_SCRIPT;
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
		flags |= MODIFY_ACTIVATION_EVENT;
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
		flags |= MODIFY_DEACTIVATION_EVENT;
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
		flags |= MODIFY_SOURCE_OBJECT;
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
		flags |= MODIFY_ACTIVE_STATUS;
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
		flags |= MODIFY_INACTIVE_STATUS;
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
		flags |= MODIFY_DCI_LIST;
	}

	/**
	 * @return the submapId
	 */
	public long getSubmapId()
	{
		return submapId;
	}

	/**
	 * @param submapId the submapId to set
	 */
	public void setSubmapId(long submapId)
	{
		this.submapId = submapId;
		flags |= MODIFY_SUBMAP_ID;
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
		flags |= MODIFY_POLLER_NODE;
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
		flags |= MODIFY_REQUIRED_POLLS;
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
		flags |= MODIFY_SERVICE_TYPE;
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
		flags |= MODIFY_IP_PROTOCOL;
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
		flags |= MODIFY_IP_PORT;
	}

	/**
	 * @return the ipAddress
	 */
	public int getIpAddress()
	{
		return ipAddress;
	}

	/**
	 * @param ipAddress the ipAddress to set
	 */
	public void setIpAddress(int ipAddress)
	{
		this.ipAddress = ipAddress;
		flags |= MODIFY_IP_ADDRESS;
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
		flags |= MODIFY_REQUEST;
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
		flags |= MODIFY_RESPONSE;
	}

	/**
	 * @return the nodeFlags
	 */
	public int getObjectFlags()
	{
		return objectFlags;
	}

	/**
	 * @param nodeFlags the nodeFlags to set
	 */
	public void setObjectFlags(int objectFlags)
	{
		this.objectFlags = objectFlags;
		flags |= MODIFY_OBJECT_FLAGS;
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
		flags |= MODIFY_IFXTABLE_POLICY;
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
		flags |= MODIFY_REPORT_DEFINITION;
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
		flags |= MODIFY_CLUSTER_RESOURCES;
	}

	/**
	 * @return the networkList
	 */
	public List<ClusterSyncNetwork> getNetworkList()
	{
		return networkList;
	}

	/**
	 * @param networkList the networkList to set
	 */
	public void setNetworkList(List<ClusterSyncNetwork> networkList)
	{
		this.networkList = networkList;
		flags |= MODIFY_CLUSTER_NETWORKS;
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
		flags |= MODIFY_PRIMARY_NAME;
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
		flags |= MODIFY_STATUS_CALCULATION;
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
		flags |= MODIFY_STATUS_CALCULATION;
	}

	/**
	 * @return the fixedPropagatedStatus
	 */
	public int getFixedPropagatedStatus()
	{
		return fixedPropagatedStatus;
	}

	/**
	 * @param fixedPropagatedStatus the fixedPropagatedStatus to set
	 */
	public void setFixedPropagatedStatus(int fixedPropagatedStatus)
	{
		this.fixedPropagatedStatus = fixedPropagatedStatus;
		flags |= MODIFY_STATUS_CALCULATION;
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
		flags |= MODIFY_STATUS_CALCULATION;
	}

	/**
	 * @return the statusTransformation
	 */
	public int[] getStatusTransformation()
	{
		return statusTransformation;
	}

	/**
	 * @param statusTransformation the statusTransformation to set
	 */
	public void setStatusTransformation(int[] statusTransformation)
	{
		this.statusTransformation = statusTransformation;
		flags |= MODIFY_STATUS_CALCULATION;
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
		flags |= MODIFY_STATUS_CALCULATION;
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
		flags |= MODIFY_STATUS_CALCULATION;
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
		flags |= MODIFY_EXPECTED_STATE;
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
		flags |= MODIFY_LINK_COLOR;
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
		flags |= MODIFY_CONNECTION_ROUTING;
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
		flags |= MODIFY_DISCOVERY_RADIUS;
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
		flags |= MODIFY_HEIGHT;
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
		flags |= MODIFY_FILTER;
	}
}
