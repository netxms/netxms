/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
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
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.base.GeoLocation;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.base.NXCommon;
import org.netxms.base.PostalAddress;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.constants.AgentCompressionMode;
import org.netxms.client.constants.CertificateMappingMethod;
import org.netxms.client.constants.GeoLocationControlMode;
import org.netxms.client.constants.IcmpStatCollectionMode;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.constants.SensorDeviceClass;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.ConditionDciInfo;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.objects.ClusterResource;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.client.users.ResponsibleUser;

/**
 * This class is used to hold data for NXCSession.modifyObject()
 */
public class NXCObjectModificationData
{
   private Long objectId;
   private String name;
   private String primaryName;
   private String alias;
   private String nameOnMap;
   private Collection<AccessListElement> acl;
   private Boolean inheritAccessRights;
   private Map<String, CustomAttribute> customAttributes;
   private String autoBindFilter;
   private Integer version;
   private Integer agentPort;
   private String agentSecret;
   private Long agentProxy;
   private Integer snmpPort;
   private SnmpVersion snmpVersion;
   private Integer snmpAuthMethod;
   private Integer snmpPrivMethod;
   private String snmpAuthName;
   private String snmpAuthPassword;
   private String snmpPrivPassword;
   private Long snmpProxy;
   private Long mqttProxy;
   private Long icmpProxy;
   private Long webServiceProxy;
   private Long[] trustedObjects;
   private GeoLocation geolocation;
   private InetAddress primaryIpAddress;
   private MapLayoutAlgorithm mapLayout;
   private UUID mapBackground;
   private GeoLocation mapBackgroundLocation;
   private Integer mapBackgroundZoom;
   private Integer mapBackgroundColor;
   private UUID mapImage;
   private Collection<NetworkMapElement> mapElements;
   private Collection<NetworkMapLink> mapLinks;
   private Integer columnCount;
   private Collection<DashboardElement> dashboardElements;
   private String script;
   private Integer activationEvent;
   private Integer deactivationEvent;
   private Long sourceObject;
   private Integer activeStatus;
   private Integer inactiveStatus;
   private List<ConditionDciInfo> dciList;
   private Long drillDownObjectId;
   private Long pollerNode;
   private Integer requiredPolls;
   private Integer serviceType;
   private Integer ipProtocol;
   private Integer ipPort;
   private InetAddressEx ipAddress;
   private String request;
   private String response;
   private Integer objectFlags;
   private Integer objectFlagsMask;
   private Integer ifXTablePolicy;
   private String reportDefinition;
   private List<ClusterResource> resourceList;
   private List<InetAddressEx> networkList;
   private Integer statusCalculationMethod;
   private Integer statusPropagationMethod;
   private ObjectStatus fixedPropagatedStatus;
   private Integer statusShift;
   private ObjectStatus[] statusTransformation;
   private Integer statusSingleThreshold;
   private int[] statusThresholds;
   private Integer expectedState;
   private Integer linkColor;
   private Integer connectionRouting;
   private Integer networkMapLinkWidth;
   private Integer networkMapLinkStyle;
   private Integer discoveryRadius;
   private Integer height;
   private String filter;
   private String linkStylingScript;
   private Long peerGatewayId;
   private List<InetAddressEx> localNetworks;
   private List<InetAddressEx> remoteNetworks;
   private PostalAddress postalAddress;
   private AgentCacheMode agentCacheMode;
   private AgentCompressionMode agentCompressionMode;
   private MapObjectDisplayMode mapObjectDisplayMode;
   private Long physicalContainerObjectId;
   private UUID rackImageFront;
   private UUID rackImageRear;
   private Short rackPosition;
   private Short rackHeight;
   private RackOrientation rackOrientation;
   private Long[] dashboards;
   private Boolean rackNumberingTopBottom;
   private Long controllerId;
   private Long chassisId;
   private Long sshProxy;
   private String sshLogin;
   private String sshPassword;
   private Integer sshPort;
   private Integer sshKeyId;
   private Long vncProxy;
   private String vncPassword;
   private Integer vncPort;
   private Long[] zoneProxies;
   private List<ObjectUrl> urls;
   private List<Long> seedObjectIds;
   private MacAddress macAddress;
   private SensorDeviceClass deviceClass;
   private String vendor;
   private String model;
   private String serialNumber;
   private String deviceAddress;
   private String metaType;
   private Long sensorProxy;
   private List<PassiveRackElement> passiveElements;
   private List<ResponsibleUser> responsibleUsers;
   private IcmpStatCollectionMode icmpStatCollectionMode;
   private List<InetAddress> icmpTargets;
   private String chassisPlacement;
   private Integer etherNetIPPort;
   private Long etherNetIPProxy;
   private Integer modbusTcpPort;
   private Short modbusUnitId;
   private Long modbusProxy;
   private CertificateMappingMethod certificateMappingMethod;
   private String certificateMappingData;
   private Integer categoryId;
   private GeoLocationControlMode geoLocationControlMode;
   private long[] geoAreas;
   private Integer instanceDiscoveryMethod;
   private String instanceDiscoveryData;
   private String instanceDiscoveryFilter;
   private String autoBindFilter2;
   private Integer autoBindFlags;
   private Integer objectStatusThreshold;
   private Integer dciStatusThreshold;
   private Long sourceNode;
   private String syslogCodepage;
   private String snmpCodepage;
   private Integer displayPriority;
   private Integer mapWidth;
   private Integer mapHeight;
   private Long expectedCapabilities;
   private String dashboardNameTemplate;

   /**
    * Constructor for creating modification data for given object
    *
    * @param objectId Object ID
    */
   public NXCObjectModificationData(long objectId)
   {
      this.objectId = objectId;
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
   }

   /**
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
   }

   /**
    * @param alias the alias to set
    */
   public void setAlias(String alias)
   {
      this.alias = alias;
   }

   /**
    * Get object's name on network map.
    *
    * @return object's name on network map
    */
   public String getNameOnMap()
   {
      return nameOnMap;
   }

   /**
    * Set object's name on network map. Set to empty string to use object's name.
    *
    * @param name new object's name on network map
    */
   public void setNameOnMap(String name)
   {
      this.nameOnMap = name;
   }

   /**
    * @return the acl
    */
   public Collection<AccessListElement> getACL()
   {
      return acl;
   }

   /**
    * @param acl the acl to set
    */
   public void setACL(Collection<AccessListElement> acl)
   {
      this.acl = (acl == null) ? new ArrayList<AccessListElement>(0) : acl;
   }

   /**
    * @return the inheritAccessRights
    */
   public Boolean isInheritAccessRights()
   {
      return inheritAccessRights;
   }

   /**
    * @param inheritAccessRights the inheritAccessRights to set
    */
   public void setInheritAccessRights(boolean inheritAccessRights)
   {
      this.inheritAccessRights = inheritAccessRights;
   }

   /**
    * @return the customAttributes
    */
   public Map<String, CustomAttribute> getCustomAttributes()
   {
      return customAttributes;
   }

   /**
    * @param customAttributes the customAttributes to set
    */
   public void setCustomAttributes(Map<String, CustomAttribute> customAttributes)
   {
      this.customAttributes = customAttributes;
   }

   /**
    * @return the autoApplyFilter
    */
   public String getAutoBindFilter()
   {
      return autoBindFilter;
   }

   /**
    * @param autoBindFilter the autoApplyFilter to set
    */
   public void setAutoBindFilter(String autoBindFilter)
   {
      this.autoBindFilter = autoBindFilter;
   }

   /**
    * @return the version
    */
   public Integer getVersion()
   {
      return version;
   }

   /**
    * @param version the version to set
    */
   public void setVersion(int version)
   {
      this.version = version;
   }

   public Integer getAgentPort()
   {
      return agentPort;
   }

   public void setAgentPort(int agentPort)
   {
      this.agentPort = agentPort;
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
   }

   /**
    * @return the agentProxy
    */
   public Long getAgentProxy()
   {
      return agentProxy;
   }

   /**
    * @param agentProxy the agentProxy to set
    */
   public void setAgentProxy(long agentProxy)
   {
      this.agentProxy = agentProxy;
   }

   /**
    * @return the snmpVersion
    */
   public SnmpVersion getSnmpVersion()
   {
      return snmpVersion;
   }

   /**
    * @param snmpVersion the snmpVersion to set
    */
   public void setSnmpVersion(SnmpVersion snmpVersion)
   {
      this.snmpVersion = snmpVersion;
   }

   /**
    * @return the snmpAuthMethod
    */
   public Integer getSnmpAuthMethod()
   {
      return snmpAuthMethod;
   }

   /**
    * @return the snmpPrivMethod
    */
   public Integer getSnmpPrivMethod()
   {
      return snmpPrivMethod;
   }

   /**
    * @return the snmpAuthName
    */
   public String getSnmpAuthName()
   {
      return snmpAuthName;
   }

   /**
    * @return the snmpAuthPassword
    */
   public String getSnmpAuthPassword()
   {
      return snmpAuthPassword;
   }

   /**
    * @return the snmpPrivPassword
    */
   public String getSnmpPrivPassword()
   {
      return snmpPrivPassword;
   }

   /**
    * Set SNMP authentication information.
    *
    * @param authName authentication name (user name for SNMPv3 or community for SNMP v1/2)
    * @param authMethod SNMPv3 authentication method
    * @param authPassword SNMPv3 authentication password
    * @param privMethod SNMPv3 encryption method
    * @param privPassword SNMPv3 encryption password
    */
   public void setSnmpAuthentication(String authName, int authMethod, String authPassword, int privMethod, String privPassword)
   {
      this.snmpAuthName = authName;
      this.snmpAuthMethod = authMethod;
      this.snmpAuthPassword = authPassword;
      this.snmpPrivMethod = privMethod;
      this.snmpPrivPassword = privPassword;
   }

   /**
    * Set SNMP authentication information - simplified version that can be used for setting SNMP v1/2 community string. Will set
    * authentication and encryption passwords to empty strings and authentication and encryption methods to NONE.
    *
    * @param authName authentication name (user name for SNMPv3 or community for SNMP v1/2)
    */
   public void setSnmpAuthentication(String authName)
   {
      setSnmpAuthentication(authName, 0, "", 0, "");
   }

   /**
    * @return the snmpProxy
    */
   public Long getSnmpProxy()
   {
      return snmpProxy;
   }

   /**
    * @param snmpProxy the snmpProxy to set
    */
   public void setSnmpProxy(long snmpProxy)
   {
      this.snmpProxy = snmpProxy;
   }

   /**
    * Get MQTT proxy node ID
    *
    * @return MQTT proxy node ID
    */
   public Long getMqttProxy()
   {
      return mqttProxy;
   }

   /**
    * Set MQTT proxy node ID.
    * 
    * @param mqttProxy MQTT proxy node ID
    */
   public void setMqttProxy(long mqttProxy)
   {
      this.mqttProxy = mqttProxy;
   }

   /**
    * @return the icmpProxy
    */
   public Long getIcmpProxy()
   {
      return icmpProxy;
   }

   /**
    * @param icmpProxy the icmpProxy to set
    */
   public void setIcmpProxy(long icmpProxy)
   {
      this.icmpProxy = icmpProxy;
   }

   /**
    * Get ID of web service proxy node.
    *
    * @return ID of web service proxy node
    */
   public Long getWebServiceProxy()
   {
      return webServiceProxy;
   }

   /**
    * Set ID of web service proxy node.
    *
    * @param webServiceProxy ID of web service proxy node
    */
   public void setWebServiceProxy(long webServiceProxy)
   {
      this.webServiceProxy = webServiceProxy;
   }

   /**
    * Get trusted objects
    *
    * @return trusted object list
    */
   public Long[] getTrustedObjects()
   {
      return trustedObjects;
   }

   /**
    * Set trusted objects
    *
    * @param trustedObjects trusted objects list
    */
   public void setTrustedObjects(Long[] trustedObjects)
   {
      this.trustedObjects = trustedObjects;
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
   }

   /**
    * @return the snmpPort
    */
   public Integer getSnmpPort()
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
   }

   /**
    * @return the mapBackground
    */
   public UUID getMapBackground()
   {
      return mapBackground;
   }

   /**
    * @param mapBackground
    */
   /**
    * @param mapBackground         The mapBackground to set
    * @param mapBackgroundLocation The mapBackgroundLocation to set
    * @param mapBackgroundZoom     The mapBackgroundZoom level to set
    * @param mapBackgroundColor    The mapBackgroundColor to set
    */
   public void setMapBackground(UUID mapBackground, GeoLocation mapBackgroundLocation, int mapBackgroundZoom,
         int mapBackgroundColor)
   {
      this.mapBackground = mapBackground == null ? NXCommon.EMPTY_GUID : mapBackground;
      this.mapBackgroundLocation = mapBackgroundLocation;
      this.mapBackgroundZoom = mapBackgroundZoom;
      this.mapBackgroundColor = mapBackgroundColor;
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
    * @param elements Network map elements
    * @param links    Network map links
    */
   public void setMapContent(Collection<NetworkMapElement> elements, Collection<NetworkMapLink> links)
   {
      mapElements = elements;
      mapLinks = links;
   }

   /**
    * Get map image.
    *
    * @return map image
    */
   public UUID getMapImage()
   {
      return mapImage;
   }

   /**
    * Set new map image
    *
    * @param image new map image
    */
   public void setMapImage(UUID image)
   {
      mapImage = image == null ? NXCommon.EMPTY_GUID : image;
   }

   /**
    * @return the columnCount
    */
   public Integer getColumnCount()
   {
      return columnCount;
   }

   /**
    * @param columnCount the columnCount to set
    */
   public void setColumnCount(int columnCount)
   {
      this.columnCount = columnCount;
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
   }

   /**
    * @return the activationEvent
    */
   public Integer getActivationEvent()
   {
      return activationEvent;
   }

   /**
    * @param activationEvent the activationEvent to set
    */
   public void setActivationEvent(int activationEvent)
   {
      this.activationEvent = activationEvent;
   }

   /**
    * @return the deactivationEvent
    */
   public Integer getDeactivationEvent()
   {
      return deactivationEvent;
   }

   /**
    * @param deactivationEvent the deactivationEvent to set
    */
   public void setDeactivationEvent(int deactivationEvent)
   {
      this.deactivationEvent = deactivationEvent;
   }

   /**
    * @return the sourceObject
    */
   public Long getSourceObject()
   {
      return sourceObject;
   }

   /**
    * @param sourceObject the sourceObject to set
    */
   public void setSourceObject(long sourceObject)
   {
      this.sourceObject = sourceObject;
   }

   /**
    * @return the activeStatus
    */
   public Integer getActiveStatus()
   {
      return activeStatus;
   }

   /**
    * @param activeStatus the activeStatus to set
    */
   public void setActiveStatus(int activeStatus)
   {
      this.activeStatus = activeStatus;
   }

   /**
    * @return the inactiveStatus
    */
   public Integer getInactiveStatus()
   {
      return inactiveStatus;
   }

   /**
    * @param inactiveStatus the inactiveStatus to set
    */
   public void setInactiveStatus(int inactiveStatus)
   {
      this.inactiveStatus = inactiveStatus;
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
   }

   /**
    * @return the submapId
    */
   public Long getDrillDownObjectId()
   {
      return drillDownObjectId;
   }

   /**
    * @param drillDownObjectId the drillDownObjectId to set
    */
   public void setDrillDownObjectId(long drillDownObjectId)
   {
      this.drillDownObjectId = drillDownObjectId;
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
   public Integer getMapBackgroundZoom()
   {
      return mapBackgroundZoom;
   }

   /**
    * @return the pollerNode
    */
   public Long getPollerNode()
   {
      return pollerNode;
   }

   /**
    * @param pollerNode the pollerNode to set
    */
   public void setPollerNode(long pollerNode)
   {
      this.pollerNode = pollerNode;
   }

   /**
    * @return the requiredPolls
    */
   public Integer getRequiredPolls()
   {
      return requiredPolls;
   }

   /**
    * @param requiredPolls the requiredPolls to set
    */
   public void setRequiredPolls(int requiredPolls)
   {
      this.requiredPolls = requiredPolls;
   }

   /**
    * @return the serviceType
    */
   public Integer getServiceType()
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
   public Integer getIpProtocol()
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
   public Integer getIpPort()
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
    * Get object flags
    *
    * @return object flags
    */
   public Integer getObjectFlags()
   {
      return objectFlags;
   }

   /**
    * Get object flags mask
    *
    * @return the object flags mask
    */
   public Integer getObjectFlagsMask()
   {
      return objectFlagsMask;
   }

   /**
    * Set object flags
    *
    * @param objectFlags Object flags
    */
   public void setObjectFlags(int objectFlags)
   {
      setObjectFlags(objectFlags, 0xFFFFFFFF);
   }

   /**
    * Set selected object flags. Only flags with corresponding mask bits set will be changed.
    *
    * @param objectFlags     new object flags
    * @param objectFlagsMask object flag mask
    */
   public void setObjectFlags(int objectFlags, int objectFlagsMask)
   {
      this.objectFlags = objectFlags;
      this.objectFlagsMask = objectFlagsMask;
   }

   /**
    * @return the ifXTablePolicy
    */
   public Integer getIfXTablePolicy()
   {
      return ifXTablePolicy;
   }

   /**
    * @param ifXTablePolicy the ifXTablePolicy to set
    */
   public void setIfXTablePolicy(int ifXTablePolicy)
   {
      this.ifXTablePolicy = ifXTablePolicy;
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
   }

   /**
    * Set report definition from file.
    *
    * @param file file containing report definition
    * @throws IOException           if file I/O error occurs
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
    * @return the statusCalculationMethod
    */
   public Integer getStatusCalculationMethod()
   {
      return statusCalculationMethod;
   }

   /**
    * @param statusCalculationMethod the statusCalculationMethod to set
    */
   public void setStatusCalculationMethod(int statusCalculationMethod)
   {
      this.statusCalculationMethod = statusCalculationMethod;
   }

   /**
    * @return the statusPropagationMethod
    */
   public Integer getStatusPropagationMethod()
   {
      return statusPropagationMethod;
   }

   /**
    * @param statusPropagationMethod the statusPropagationMethod to set
    */
   public void setStatusPropagationMethod(int statusPropagationMethod)
   {
      this.statusPropagationMethod = statusPropagationMethod;
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
   }

   /**
    * @return the statusShift
    */
   public Integer getStatusShift()
   {
      return statusShift;
   }

   /**
    * @param statusShift the statusShift to set
    */
   public void setStatusShift(int statusShift)
   {
      this.statusShift = statusShift;
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
   }

   /**
    * @return the statusSingleThreshold
    */
   public Integer getStatusSingleThreshold()
   {
      return statusSingleThreshold;
   }

   /**
    * @param statusSingleThreshold the statusSingleThreshold to set
    */
   public void setStatusSingleThreshold(int statusSingleThreshold)
   {
      this.statusSingleThreshold = statusSingleThreshold;
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
   }

   /**
    * @return the expectedState
    */
   public Integer getExpectedState()
   {
      return expectedState;
   }

   /**
    * @param expectedState the expectedState to set
    */
   public void setExpectedState(int expectedState)
   {
      this.expectedState = expectedState;
   }

   /**
    * @return the linkColor
    */
   public Integer getLinkColor()
   {
      return linkColor;
   }

   /**
    * @param linkColor the linkColor to set
    */
   public void setLinkColor(int linkColor)
   {
      this.linkColor = linkColor;
   }

   /**
    * @return the connectionRouting
    */
   public Integer getConnectionRouting()
   {
      return connectionRouting;
   }

   /**
    * @param connectionRouting the connectionRouting to set
    */
   public void setConnectionRouting(int connectionRouting)
   {
      this.connectionRouting = connectionRouting;
   }

   /**
    * @return the networkMapLinkWidth
    */
   public Integer getNetworkMapLinkWidth()
   {
      return networkMapLinkWidth;
   }

   /**
    * @param networkMapLinkWidth the networkMapLinkWidth to set
    */
   public void setNetworkMapLinkWidth(Integer networkMapLinkWidth)
   {
      this.networkMapLinkWidth = networkMapLinkWidth;
   }

   /**
    * @return the networkMapLinkStyle
    */
   public Integer getNetworkMapLinkStyle()
   {
      return networkMapLinkStyle;
   }

   /**
    * @param networkMapLinkStyle the networkMapLinkStyle to set
    */
   public void setNetworkMapLinkStyle(Integer networkMapLinkStyle)
   {
      this.networkMapLinkStyle = networkMapLinkStyle;
   }

   /**
    * @return the mapBackgroundColor
    */
   public Integer getMapBackgroundColor()
   {
      return mapBackgroundColor;
   }

   /**
    * @return the discoveryRadius
    */
   public final Integer getDiscoveryRadius()
   {
      return discoveryRadius;
   }

   /**
    * @param discoveryRadius the discoveryRadius to set
    */
   public final void setDiscoveryRadius(int discoveryRadius)
   {
      this.discoveryRadius = discoveryRadius;
   }

   /**
    * @return the height
    */
   public Integer getHeight()
   {
      return height;
   }

   /**
    * @param height the height to set
    */
   public void setHeight(int height)
   {
      this.height = height;
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
   }

   /**
    * @return the linkStylingScript
    */
   public String getLinkStylingScript()
   {
      return linkStylingScript;
   }

   /**
    * @param linkStylingScript the linkStylingScript to set
    */
   public void setLinkStylingScript(String linkStylingScript)
   {
      this.linkStylingScript = linkStylingScript;
   }

   /**
    * @return the peerGatewayId
    */
   public Long getPeerGatewayId()
   {
      return peerGatewayId;
   }

   /**
    * @param peerGatewayId the peerGatewayId to set
    */
   public void setPeerGatewayId(long peerGatewayId)
   {
      this.peerGatewayId = peerGatewayId;
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
    * @param localNetworks  the localNetworks to set
    * @param remoteNetworks the remoteNetworks to set
    */
   public void setVpnNetworks(List<InetAddressEx> localNetworks, List<InetAddressEx> remoteNetworks)
   {
      this.localNetworks = localNetworks;
      this.remoteNetworks = remoteNetworks;
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
   }

   /**
    * Get agent cache mode
    *
    * @return agent cache mode
    */
   public AgentCacheMode getAgentCacheMode()
   {
      return agentCacheMode;
   }

   /**
    * Set agent cache mode
    *
    * @param agentCacheMode new agent cache mode
    */
   public void setAgentCacheMode(AgentCacheMode agentCacheMode)
   {
      this.agentCacheMode = agentCacheMode;
   }

   /**
    * Get agent compression mode
    *
    * @return agent compression mode
    */
   public AgentCompressionMode getAgentCompressionMode()
   {
      return agentCompressionMode;
   }

   /**
    * Set agent compression mode
    *
    * @param agentCompressionMode new agent compression mode
    */
   public void setAgentCompressionMode(AgentCompressionMode agentCompressionMode)
   {
      this.agentCompressionMode = agentCompressionMode;
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
   }

   /**
    * @return the physicalContainerObjectId
    */
   public Long getPhysicalContainerObjectId()
   {
      return physicalContainerObjectId;
   }

   /**
    * @return the front rackImage
    */
   public UUID getFrontRackImage()
   {
      return rackImageFront;
   }

   /**
    * @return the rear rackImage
    */
   public UUID getRearRackImage()
   {
      return rackImageRear;
   }

   /**
    * @return the rackPosition
    */
   public Short getRackPosition()
   {
      return rackPosition;
   }

   /**
    * @return the rackHeight
    */
   public Short getRackHeight()
   {
      return rackHeight;
   }

   /**
    * Get rack orientation
    *
    * @return rack orientation
    */
   public RackOrientation getRackOrientation()
   {
      return rackOrientation;
   }

   /**
    * Set rack placement data
    *
    * @param rackImageFront The front rack image to set
    * @param rackImageRear  The rear rack image to set
    * @param rackPosition   The rack position to set
    * @param rackHeight     the rack height to set
    * @param rackOrientation Rack orientation (front/rear)
    */
   public void setRackPlacement(UUID rackImageFront, UUID rackImageRear, short rackPosition, short rackHeight,
         RackOrientation rackOrientation)
   {
      this.rackImageFront = rackImageFront == null ? NXCommon.EMPTY_GUID : rackImageFront;
      this.rackImageRear = rackImageRear == null ? NXCommon.EMPTY_GUID : rackImageRear;
      this.rackPosition = rackPosition;
      this.rackHeight = rackHeight;
      this.rackOrientation = rackOrientation;
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
   }

   /**
    * @return the rackNumberingTopBottom
    */
   public Boolean isRackNumberingTopBottom()
   {
      return rackNumberingTopBottom;
   }

   /**
    * @param rackNumberingTopBottom the rackNumberingTopBottom to set
    */
   public void setRackNumberingTopBottom(boolean rackNumberingTopBottom)
   {
      this.rackNumberingTopBottom = rackNumberingTopBottom;
   }

   /**
    * @return the controllerId
    */
   public Long getControllerId()
   {
      return controllerId;
   }

   /**
    * @param controllerId the controllerId to set
    */
   public void setControllerId(long controllerId)
   {
      this.controllerId = controllerId;
   }

   /**
    * @return the chassisId
    */
   public Long getChassisId()
   {
      return chassisId;
   }

   /**
    * @param chassisId the chassisId to set
    */
   public void setChassisId(long chassisId)
   {
      this.chassisId = chassisId;
   }

   /**
    * @return the sshProxy
    */
   public Long getSshProxy()
   {
      return sshProxy;
   }

   /**
    * @param sshProxy the sshProxy to set
    */
   public void setSshProxy(long sshProxy)
   {
      this.sshProxy = sshProxy;
   }

   /**
    * @return the sshKeyId
    */
   public Integer getSshKeyId()
   {
      return sshKeyId;
   }

   /**
    * @param sshKeyId the sshKeyId to set
    */
   public void setSshKeyId(int sshKeyId)
   {
      this.sshKeyId = sshKeyId;
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
   }

   public Integer getSshPort()
   {
      return sshPort;
   }

   public void setSshPort(int sshPort)
   {
      this.sshPort = sshPort;
   }

   /**
    * @return the vncProxy
    */
   public Long getVncProxy()
   {
      return vncProxy;
   }

   /**
    * @param vncProxy the vncProxy to set
    */
   public void setVncProxy(Long vncProxy)
   {
      this.vncProxy = vncProxy;
   }

   /**
    * @return the vncPassword
    */
   public String getVncPassword()
   {
      return vncPassword;
   }

   /**
    * @param vncPassword the vncPassword to set
    */
   public void setVncPassword(String vncPassword)
   {
      this.vncPassword = vncPassword;
   }

   /**
    * @return the vncPort
    */
   public Integer getVncPort()
   {
      return vncPort;
   }

   /**
    * @param vncPort the vncPort to set
    */
   public void setVncPort(Integer vncPort)
   {
      this.vncPort = vncPort;
   }

   /**
    * @return the zoneProxies
    */
   public Long[] getZoneProxies()
   {
      return zoneProxies;
   }

   /**
    * @param zoneProxies the zoneProxies to set
    */
   public void setZoneProxies(Long[] zoneProxies)
   {
      this.zoneProxies = zoneProxies;
   }

   /**
    * @return urls
    */
   public List<ObjectUrl> getUrls()
   {
      return urls;
   }

   /**
    * Set URL list
    *
    * @param urls new URL list
    */
   public void setUrls(List<ObjectUrl> urls)
   {
      this.urls = urls;
   }

   /**
    * @return the seedObjectIds
    */
   public Long[] getSeedObjectIds()
   {
      return seedObjectIds == null ? null : seedObjectIds.toArray(new Long[seedObjectIds.size()]);
   }

   /**
    * @return the seedObjectIds
    */
   public List<Long> getSeedObjectIdsAsList()
   {
      return seedObjectIds;
   }

   /**
    * @param seedObjectIds the seed node object Ids to set
    */
   public void setSeedObjectIds(List<Long> seedObjectIds)
   {
      this.seedObjectIds = seedObjectIds;
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
    * @return the deviceClass
    */
   public SensorDeviceClass getDeviceClass()
   {
      return deviceClass;
   }

   /**
    * @param deviceClass the deviceClass to set
    */
   public void setDeviceClass(SensorDeviceClass deviceClass)
   {
      this.deviceClass = deviceClass;
   }

   /**
    * @return the vendor
    */
   public String getVendor()
   {
      return vendor;
   }

   /**
    * @param vendor the vendor to set
    */
   public void setVendor(String vendor)
   {
      this.vendor = vendor;
   }

   /**
    * @return the model
    */
   public String getModel()
   {
      return model;
   }

   /**
    * @param model the model to set
    */
   public void setModel(String model)
   {
      this.model = model;
   }

   /**
    * @return the serialNumber
    */
   public String getSerialNumber()
   {
      return serialNumber;
   }

   /**
    * @param serialNumber the serialNumber to set
    */
   public void setSerialNumber(String serialNumber)
   {
      this.serialNumber = serialNumber;
   }

   /**
    * @return the deviceAddress
    */
   public String getDeviceAddress()
   {
      return deviceAddress;
   }

   /**
    * @param deviceAddress the deviceAddress to set
    */
   public void setDeviceAddress(String deviceAddress)
   {
      this.deviceAddress = deviceAddress;
   }

   /**
    * @return the metaType
    */
   public String getMetaType()
   {
      return metaType;
   }

   /**
    * @param metaType the metaType to set
    */
   public void setMetaType(String metaType)
   {
      this.metaType = metaType;
   }

   public void setSensorProxy(long proxyNode)
   {
      this.sensorProxy = proxyNode;
   }

   public Long getGatewayNodeId()
   {
      return sensorProxy;
   }

   /**
    * @return the passiveElementsConfig
    */
   public List<PassiveRackElement> getPassiveElements()
   {
      return passiveElements;
   }

   /**
    * Set passive rack elements configuration
    *
    * @param passiveElements XMS configuration of passive rack elements
    */
   public void setPassiveElements(List<PassiveRackElement> passiveElements)
   {
      this.passiveElements = passiveElements;
   }

   /**
    * @return responsible users list
    */
   public List<ResponsibleUser> getResponsibleUsers()
   {
      return responsibleUsers;
   }

   /**
    * Set responsible users
    *
    * @param responsibleUsers to set
    */
   public void setResponsibleUsers(List<ResponsibleUser> responsibleUsers)
   {
      this.responsibleUsers = responsibleUsers;
   }

   /**
    * Get ICMP statistic collection mode
    * 
    * @return ICMP statistic collection mode
    */
   public IcmpStatCollectionMode getIcmpStatCollectionMode()
   {
      return icmpStatCollectionMode;
   }

   /**
    * Set ICMP statistic collection mode
    * 
    * @param mode new ICMP statistic collection mode
    */
   public void setIcmpStatCollectionMode(IcmpStatCollectionMode mode)
   {
      this.icmpStatCollectionMode = mode;
   }

   /**
    * Get additional ICMP poll targets
    * 
    * @return additional ICMP poll targets
    */
   public List<InetAddress> getIcmpTargets()
   {
      return icmpTargets;
   }

   /**
    * Set additional ICMP poll targets
    * 
    * @param icmpTargets new list of additional ICMP poll targets
    */
   public void setIcmpTargets(Collection<InetAddress> icmpTargets)
   {
      this.icmpTargets = new ArrayList<InetAddress>(icmpTargets);
   }

   /**
    * Set physical container for object
    * Will be set together with CHASSIS_PLACEMENT or RACK_PLACEMENT so no need to set fields
    * 
    * @param physicalContainerObjectId bject ID of physical container
    */
   public void setPhysicalContainer(long physicalContainerObjectId)
   {
      this.physicalContainerObjectId = physicalContainerObjectId;
   }

   /**
    * Set chassis placement information
    * 
    * @param placementConfig chassis placement XML config
    */
   public void setChassisPlacement(String placementConfig)
   {
      chassisPlacement = placementConfig;
   }

   /**
    * Get chassis placement configuration.
    *
    * @return chassis placement configuration
    */
   public String getChassisPlacement()
   {
      return chassisPlacement;
   }

   /**
    * @return the etherNetIPPort
    */
   public Integer getEtherNetIPPort()
   {
      return etherNetIPPort;
   }

   /**
    * @param etherNetIPPort the etherNetIPPort to set
    */
   public void setEtherNetIPPort(int etherNetIPPort)
   {
      this.etherNetIPPort = etherNetIPPort;
   }

   /**
    * @return the etherNetIPProxy
    */
   public Long getEtherNetIPProxy()
   {
      return etherNetIPProxy;
   }

   /**
    * @param etherNetIPProxy the etherNetIPProxy to set
    */
   public void setEtherNetIPProxy(long etherNetIPProxy)
   {
      this.etherNetIPProxy = etherNetIPProxy;
   }

   /**
    * @return the modbusTcpPort
    */
   public Integer getModbusTcpPort()
   {
      return modbusTcpPort;
   }

   /**
    * @param modbusTcpPort the modbusTcpPort to set
    */
   public void setModbusTcpPort(Integer modbusTcpPort)
   {
      this.modbusTcpPort = modbusTcpPort;
   }

   /**
    * @return the modbusUnitId
    */
   public Short getModbusUnitId()
   {
      return modbusUnitId;
   }

   /**
    * @param modbusUnitId the modbusUnitId to set
    */
   public void setModbusUnitId(Short modbusUnitId)
   {
      this.modbusUnitId = modbusUnitId;
   }

   /**
    * @return the modbusProxy
    */
   public Long getModbusProxy()
   {
      return modbusProxy;
   }

   /**
    * @param modbusProxy the modbusProxy to set
    */
   public void setModbusProxy(Long modbusProxy)
   {
      this.modbusProxy = modbusProxy;
   }

   /**
    * Set certificate mapping method and data.
    *
    * @param method mapping method
    * @param data mapping data (method dependent, can be null)
    */
   public void setCertificateMapping(CertificateMappingMethod method, String data)
   {
      certificateMappingMethod = method;
      certificateMappingData = data;
   }

   /**
    * Get certificate mapping method.
    *
    * @return certificate mapping method
    */
   public CertificateMappingMethod getCertificateMappingMethod()
   {
      return certificateMappingMethod;
   }

   /**
    * Get certificate mapping data.
    *
    * @return certificate mapping data
    */
   public String getCertificateMappingData()
   {
      return certificateMappingData;
   }

   /**
    * Get category ID.
    * 
    * @return category ID
    */
   public Integer getCategoryId()
   {
      return categoryId;
   }

   /**
    * Set category ID.
    *
    * @param categoryId new category ID
    */
   public void setCategoryId(int categoryId)
   {
      this.categoryId = categoryId;
   }

   /**
    * Get geolocation control mode
    * 
    * @return geolocation control mode
    */
   public GeoLocationControlMode getGeoLocationControlMode()
   {
      return geoLocationControlMode;
   }

   /**
    * Set geolocation control mode
    *
    * @param geoLocationControlMode new geolocation control mode
    */
   public void setGeoLocationControlMode(GeoLocationControlMode geoLocationControlMode)
   {
      this.geoLocationControlMode = geoLocationControlMode;
   }

   /**
    * Get geo areas for object
    *
    * @return geo areas for object
    */
   public long[] getGeoAreas()
   {
      return geoAreas;
   }

   /**
    * Set geo areas for object
    *
    * @param geoAreas new geo areas for object
    */
   public void setGeoAreas(long[] geoAreas)
   {
      this.geoAreas = geoAreas;
   }

   /**
    * @return the instanceDiscoveryMethod
    */
   public final Integer getInstanceDiscoveryMethod()
   {
      return instanceDiscoveryMethod;
   }

   /**
    * @param instanceDiscoveryMethod the instanceDiscoveryMethod to set
    */
   public final void setInstanceDiscoveryMethod(int instanceDiscoveryMethod)
   {
      this.instanceDiscoveryMethod = instanceDiscoveryMethod;
   }

   /**
    * @return the instanceDiscoveryData
    */
   public final String getInstanceDiscoveryData()
   {
      return instanceDiscoveryData;
   }

   /**
    * @param instanceDiscoveryData the instanceDiscoveryData to set
    */
   public final void setInstanceDiscoveryData(String instanceDiscoveryData)
   {
      this.instanceDiscoveryData = instanceDiscoveryData;
   }

   /**
    * @return the instanceDiscoveryFilter
    */
   public final String getInstanceDiscoveryFilter()
   {
      return instanceDiscoveryFilter;
   }

   /**
    * @param instanceDiscoveryFilter the instanceDiscoveryFilter to set
    */
   public final void setInstanceDiscoveryFilter(String instanceDiscoveryFilter)
   {
      this.instanceDiscoveryFilter = instanceDiscoveryFilter;
   }

   /**
    * @return the autoBindFilter2
    */
   public String getAutoBindFilter2()
   {
      return autoBindFilter2;
   }

   /**
    * @param autoBindFilter2 the autoBindFilter2 to set
    */
   public void setAutoBindFilter2(String autoBindFilter2)
   {
      this.autoBindFilter2 = autoBindFilter2;
   }

   /**
    * @return the autoBindFlags
    */
   public Integer getAutoBindFlags()
   {
      return autoBindFlags;
   }

   /**
    * @param autoBindFlags the autoBindFlags to set
    */
   public void setAutoBindFlags(Integer autoBindFlags)
   {
      this.autoBindFlags = autoBindFlags;
   }

   /**
    * @param objectStatusThreshold the objectStatusThreshold to set
    */
   public void setObjectStatusThreshold(Integer objectStatusThreshold)
   {
      this.objectStatusThreshold = objectStatusThreshold;
   }

   /**
    * @return the objectStatusThreshold
    */
   public Integer getObjectStatusThreshold()
   {
      return objectStatusThreshold;
   }

   /**
    * @return the dciStatusThreshold
    */
   public Integer getDciStatusThreshold()
   {
      return dciStatusThreshold;
   }

   /**
    * @param dciStatusThreshold the dciStatusThreshold to set
    */
   public void setDciStatusThreshold(Integer dciStatusThreshold)
   {
      this.dciStatusThreshold = dciStatusThreshold;
   }

   /**
    * @param sourceNode the sourceNode to set
    */
   public void setSourceNode(Long sourceNode)
   {
      this.sourceNode = sourceNode;
   }

   /**
    * @return the dciStatusThreshold
    */
   public Long getSourceNode()
   {
      return sourceNode;
   }

   /**
    * @param syslogCodepage the syslog codepage to set
    */
   public void setSyslogCodepage(String syslogCodepage)
   {
      this.syslogCodepage = syslogCodepage;
   }

   /**
    * @return the syslogCodepage
    */
   public String getSyslogCodepage()
   {
      return syslogCodepage;
   }

   /**
    * @param snmpCodepage the SNMP codepage to set
    */
   public void setSNMPCodepage(String snmpCodepage)
   {
      this.snmpCodepage = snmpCodepage;
   }

   /**
    * @return the snmpCodepage
    */
   public String getSNMPCodepage()
   {
      return snmpCodepage;
   }

   /**
    * @return the displayPriority
    */
   public Integer getDisplayPriority()
   {
      return displayPriority;
   }

   /**
    * @param displayPriority the displayPriority to set
    */
   public void setDisplayPriority(Integer displayPriority)
   {
      this.displayPriority = displayPriority;
   }

   /**
    * Set map size.
    * 
    * @param width map width in logical pixels
    * @param height map height in logical pixels
    */
   public void setMapSize(int width, int height)
   {
      mapWidth = width;
      mapHeight = height;      
   }

   /**
    * @return the mapWidth
    */
   public Integer getMapWidth()
   {
      return mapWidth;
   }

   /**
    * @return the mapHeight
    */
   public Integer getMapHeight()
   {
      return mapHeight;
   }

   /**
    * @return the expectedCapabilities
    */
   public Long getExpectedCapabilities()
   {
      return expectedCapabilities;
   }

   /**
    * @param expectedCapabilities the expectedCapabilities to set
    */
   public void setExpectedCapabilities(Long expectedCapabilities)
   {
      this.expectedCapabilities = expectedCapabilities;
   }

   /**
    * Get dashboard name template
    * 
    * @return dashboard name template
    */
   public String getDashboardNameTemplate()
   {
      return dashboardNameTemplate;
   }

   /**
    * Set dashboard name template
    * 
    * @param dashboardNameTemplate dashboard name template
    */
   public void setDashboardNameTemplate(String dashboardNameTemplate)
   {
      this.dashboardNameTemplate = dashboardNameTemplate;
   }
}
