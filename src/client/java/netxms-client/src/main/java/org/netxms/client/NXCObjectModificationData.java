/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import org.netxms.base.GeoLocation;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.base.PostalAddress;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.constants.AgentCompressionMode;
import org.netxms.client.constants.CertificateMappingMethod;
import org.netxms.client.constants.GeoLocationControlMode;
import org.netxms.client.constants.IcmpStatCollectionMode;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.RackOrientation;
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

/**
 * This class is used to hold data for NXCSession.modifyObject()
 */
public class NXCObjectModificationData
{
   // Modification flags
   public static final int NAME                   = 1;
   public static final int ACL                    = 2;
   public static final int CUSTOM_ATTRIBUTES      = 3;
   public static final int AUTOBIND_FILTER        = 4;
   public static final int LINK_COLOR             = 5;
   public static final int AUTOBIND_FLAGS         = 6;
   public static final int VERSION                = 7;
   public static final int DESCRIPTION            = 8;
   public static final int AGENT_PORT             = 9;
   public static final int AGENT_SECRET           = 10;
   public static final int SNMP_VERSION           = 11;
   public static final int SNMP_AUTH              = 12;
   public static final int AGENT_PROXY            = 13;
   public static final int SNMP_PROXY             = 14;
   public static final int TRUSTED_NODES          = 15;
   public static final int GEOLOCATION            = 16;
   public static final int PRIMARY_IP             = 17;
   public static final int SNMP_PORT              = 18;
   public static final int MAP_LAYOUT             = 19;
   public static final int MAP_BACKGROUND         = 20;
   public static final int MAP_CONTENT            = 21;
   public static final int MAP_IMAGE              = 22;
   public static final int ICMP_PROXY             = 23;
   public static final int COLUMN_COUNT           = 24;
   public static final int DASHBOARD_ELEMENTS     = 25;
   public static final int SCRIPT                 = 26;
   public static final int ACTIVATION_EVENT       = 27;
   public static final int DEACTIVATION_EVENT     = 28;
   public static final int SOURCE_OBJECT          = 29;
   public static final int ACTIVE_STATUS          = 30;
   public static final int INACTIVE_STATUS        = 31;
   public static final int DCI_LIST               = 32;
   public static final int DRILL_DOWN_OBJECT_ID   = 33;
   public static final int IP_ADDRESS             = 34;
   public static final int IP_PROTOCOL            = 35;
   public static final int IP_PORT                = 36;
   public static final int SERVICE_TYPE           = 37;
   public static final int POLLER_NODE            = 38;
   public static final int REQUIRED_POLLS         = 39;
   public static final int REQUEST                = 40;
   public static final int RESPONSE               = 41;
   public static final int OBJECT_FLAGS           = 42;
   public static final int IFXTABLE_POLICY        = 43;
   public static final int REPORT_DEFINITION      = 44;
   public static final int CLUSTER_RESOURCES      = 45;
   public static final int PRIMARY_NAME           = 46;
   public static final int STATUS_CALCULATION     = 47;
   public static final int CLUSTER_NETWORKS       = 48;
   public static final int EXPECTED_STATE         = 49;
   public static final int CONNECTION_ROUTING     = 50;
   public static final int DISCOVERY_RADIUS       = 51;
   public static final int HEIGHT                 = 52;
   public static final int FILTER                 = 53;
   public static final int PEER_GATEWAY           = 54;
   public static final int VPN_NETWORKS           = 55;
   public static final int POSTAL_ADDRESS         = 56;
   public static final int AGENT_CACHE_MODE       = 57;
   public static final int MAPOBJ_DISP_MODE       = 58;
   public static final int RACK_PLACEMENT         = 59;
   public static final int DASHBOARD_LIST         = 60;
   public static final int RACK_NUMB_SCHEME       = 61;
   public static final int CONTROLLER_ID          = 62;
   public static final int CHASSIS_ID             = 63;
   public static final int SSH_PROXY              = 64;
   public static final int SSH_LOGIN              = 65;
   public static final int SSH_PASSWORD           = 66;
   public static final int ZONE_PROXY_LIST        = 67;
   public static final int AGENT_COMPRESSION_MODE = 68;
   public static final int URL_LIST               = 69;
   public static final int SEED_OBJECTS           = 70;
   public static final int MAC_ADDRESS            = 71;
   public static final int DEVICE_CLASS           = 72;
   public static final int VENDOR                 = 73;
   public static final int SERIAL_NUMBER          = 74;
   public static final int DEVICE_ADDRESS         = 75;
   public static final int META_TYPE              = 76;
   public static final int SENSOR_PROXY           = 77;
   public static final int XML_CONFIG             = 78;
   public static final int SNMP_PORT_LIST         = 79;
   public static final int PASSIVE_ELEMENTS       = 80;
   public static final int RESPONSIBLE_USERS      = 81;
   public static final int ICMP_POLL_MODE         = 82;
   public static final int ICMP_POLL_TARGETS      = 83;
   public static final int CHASSIS_PLACEMENT      = 84;
   public static final int ETHERNET_IP_PORT       = 85;
   public static final int ETHERNET_IP_PROXY      = 86;
   public static final int ALIAS                  = 87;
   public static final int NAME_ON_MAP            = 88;
   public static final int CERT_MAPPING           = 89;
   public static final int CATEGORY_ID            = 90;
   public static final int GEO_AREAS              = 91;
   public static final int GEOLOCATION_CTRL_MODE  = 92;
   public static final int SSH_PORT               = 93;
   public static final int SSH_KEY_ID             = 94;

   private Set<Integer> fieldSet;
   private long objectId;
   private String name;
   private String primaryName;
   private String alias;
   private String nameOnMap;
   private AccessListElement[] acl;
   private boolean inheritAccessRights;
   private Map<String, CustomAttribute> customAttributes;
   private String autoBindFilter;
   private int version;
   private String description;
   private int agentPort;
   private String agentSecret;
   private long agentProxy;
   private int snmpPort;
   private SnmpVersion snmpVersion;
   private int snmpAuthMethod;
   private int snmpPrivMethod;
   private String snmpAuthName;
   private String snmpAuthPassword;
   private String snmpPrivPassword;
   private long snmpProxy;
   private long icmpProxy;
   private Long[] trustedNodes;
   private GeoLocation geolocation;
   private InetAddress primaryIpAddress;
   private MapLayoutAlgorithm mapLayout;
   private UUID mapBackground;
   private GeoLocation mapBackgroundLocation;
   private int mapBackgroundZoom;
   private int mapBackgroundColor;
   private UUID mapImage;
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
   private AgentCompressionMode agentCompressionMode;
   private MapObjectDisplayMode mapObjectDisplayMode;
   private long physicalContainerObjectId;
   private UUID rackImageFront;
   private UUID rackImageRear;
   private short rackPosition;
   private short rackHeight;
   private RackOrientation rackOrientation;
   private Long[] dashboards;
   private boolean rackNumberingTopBottom;
   private long controllerId;
   private long chassisId;
   private long sshProxy;
   private String sshLogin;
   private String sshPassword;
   private int sshPort;
   private int sshKeyId;
   private Long[] zoneProxies;
   private List<ObjectUrl> urls;
   private List<Long> seedObjectIds;
   private MacAddress macAddress;
   private int deviceClass;
   private String vendor;
   private String serialNumber;
   private String deviceAddress;
   private String metaType;
   private long sensorProxy;
   private String xmlConfig;
   private List<String> snmpPorts;
   private List<PassiveRackElement> passiveElements;
   private List<Long> responsibleUsers;
   private boolean isAutoBindEnabled;
   private boolean isAutoUnbindEnabled;
   private IcmpStatCollectionMode icmpStatCollectionMode;
   private List<InetAddress> icmpTargets;
   private String chassisPlacement;
   private int etherNetIPPort;
   private long etherNetIPProxy;
   private CertificateMappingMethod certificateMappingMethod;
   private String certificateMappingData;
   private int categoryId;
   private GeoLocationControlMode geoLocationControlMode;
   private long[] geoAreas;

   /**
    * Constructor for creating modification data for given object
    *
    * @param objectId Object ID
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
      fieldSet.add(ALIAS);
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
      fieldSet.add(NAME_ON_MAP);
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
    * @param autoBindFilter the autoApplyFilter to set
    */
   public void setAutoBindFilter(String autoBindFilter)
   {
      this.autoBindFilter = autoBindFilter;
      fieldSet.add(AUTOBIND_FILTER);
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
      fieldSet.add(AGENT_SECRET);
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
   public Long[] getTrustedNodes()
   {
      return trustedNodes;
   }

   /**
    * @param trustedNodes the trustedNodes to set
    */
   public void setTrustedNodes(Long[] trustedNodes)
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
    * @param elements Network map elements
    * @param links    Network map links
    */
   public void setMapContent(Collection<NetworkMapElement> elements, Collection<NetworkMapLink> links)
   {
      mapElements = elements;
      mapLinks = links;
      fieldSet.add(MAP_CONTENT);
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
      mapImage = image;
      fieldSet.add(MAP_IMAGE);
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
    * Get object flags
    *
    * @return object flags
    */
   public int getObjectFlags()
   {
      return objectFlags;
   }

   /**
    * Get object flags mask
    *
    * @return the object flags mask
    */
   public int getObjectFlagsMask()
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
    * @param localNetworks  the localNetworks to set
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
      fieldSet.add(AGENT_CACHE_MODE);
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
      fieldSet.add(AGENT_COMPRESSION_MODE);
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
    * @return the physicalContainerObjectId
    */
   public long getPhysicalContainerObjectId()
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
      this.rackImageFront = rackImageFront;
      this.rackImageRear = rackImageRear;
      this.rackPosition = rackPosition;
      this.rackHeight = rackHeight;
      this.rackOrientation = rackOrientation;
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
    * @return the sshKeyId
    */
   public int getSshKeyId()
   {
      return sshKeyId;
   }

   /**
    * @param sshKeyId the sshKeyId to set
    */
   public void setSshKeyId(int sshKeyId)
   {
      this.sshKeyId = sshKeyId;
      fieldSet.add(SSH_KEY_ID);
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

   public int getSshPort()
   {
      return sshPort;
   }

   public void setSshPort(int sshPort)
   {
      this.sshPort = sshPort;
      fieldSet.add(SSH_PORT);
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
      fieldSet.add(ZONE_PROXY_LIST);
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
      fieldSet.add(URL_LIST);
   }

   /**
    * @return the seedObjectIds
    */
   public Long[] getSeedObjectIds()
   {
      return seedObjectIds.toArray(new Long[seedObjectIds.size()]);
   }

   /**
    * @param seedObjectIds the seed node object Ids to set
    */
   public void setSeedObjectIds(List<Long> seedObjectIds)
   {
      this.seedObjectIds = seedObjectIds;
      fieldSet.add(SEED_OBJECTS);
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
      fieldSet.add(MAC_ADDRESS);
   }

   /**
    * @return the deviceClass
    */
   public int getDeviceClass()
   {
      return deviceClass;
   }

   /**
    * @param deviceClass the deviceClass to set
    */
   public void setDeviceClass(int deviceClass)
   {
      this.deviceClass = deviceClass;
      fieldSet.add(DEVICE_CLASS);
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
      fieldSet.add(VENDOR);
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
      fieldSet.add(SERIAL_NUMBER);
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
      fieldSet.add(DEVICE_ADDRESS);
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
      fieldSet.add(META_TYPE);
   }

   public void setSensorProxy(long proxyNode)
   {
      this.sensorProxy = proxyNode;
      fieldSet.add(SENSOR_PROXY);
   }

   public long getSensorProxy()
   {
      return sensorProxy;
   }

   /**
    * @return the xmlConfig
    */
   public String getXmlConfig()
   {
      return xmlConfig;
   }

   /**
    * @param xmlConfig the xmlConfig to set
    */
   public void setXmlConfig(String xmlConfig)
   {
      this.xmlConfig = xmlConfig;
      fieldSet.add(XML_CONFIG);
   }

   /**
    * Update zone snmp port list
    *
    * @param snmpPorts to set
    */
   public void setSnmpPorts(List<String> snmpPorts)
   {
      this.snmpPorts = snmpPorts;
      fieldSet.add(SNMP_PORT_LIST);
   }

   /**
    * Get the snmp port list
    *
    * @return snmp port list
    */
   public List<String> getSnmpPorts()
   {
      return snmpPorts;
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
      fieldSet.add(PASSIVE_ELEMENTS);
   }

   /**
    * @return responsible users list
    */
   public List<Long> getResponsibleUsers()
   {
      return responsibleUsers;
   }

   /**
    * Set responsible users
    *
    * @param responsibleUsers to set
    */
   public void setResponsibleUsers(List<Long> responsibleUsers)
   {
      this.responsibleUsers = responsibleUsers;
      fieldSet.add(RESPONSIBLE_USERS);
   }

   /**
    * Set abuto bind/remove options
    *
    * @param autoApply  TODO
    * @param autoUnbind TODO
    */
   public void setAutoBindFlags(boolean autoApply, boolean autoUnbind)
   {
      isAutoBindEnabled = autoApply;
      isAutoUnbindEnabled = autoUnbind;
      fieldSet.add(AUTOBIND_FLAGS);
   }

   /**
    * @return if auto bind is enabled
    */
   public boolean isAutoBindEnabled()
   {
      return isAutoBindEnabled;
   }

   /**
    * @return if auto remove is enabled
    */
   public boolean isAutoUnbindEnabled()
   {
      return isAutoUnbindEnabled;
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
      fieldSet.add(ICMP_POLL_MODE);
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
      fieldSet.add(ICMP_POLL_TARGETS);
   }

   /**
    * Set physical container for object
    * Will be set together with CHASSIS_PLACEMENT or RACK_PLACEMENT so no need to set fields
    * 
    * @param physicalContainerObjectId
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
      fieldSet.add(CHASSIS_PLACEMENT);
   }

   /**
    * @return the chassisPlacement
    */
   public String getChassisPlacement()
   {
      return chassisPlacement;
   }

   /**
    * @return the etherNetIPPort
    */
   public int getEtherNetIPPort()
   {
      return etherNetIPPort;
   }

   /**
    * @param etherNetIPPort the etherNetIPPort to set
    */
   public void setEtherNetIPPort(int etherNetIPPort)
   {
      this.etherNetIPPort = etherNetIPPort;
      fieldSet.add(ETHERNET_IP_PORT);
   }

   /**
    * @return the etherNetIPProxy
    */
   public long getEtherNetIPProxy()
   {
      return etherNetIPProxy;
   }

   /**
    * @param etherNetIPProxy the etherNetIPProxy to set
    */
   public void setEtherNetIPProxy(long etherNetIPProxy)
   {
      this.etherNetIPProxy = etherNetIPProxy;
      fieldSet.add(ETHERNET_IP_PROXY);
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
      fieldSet.add(CERT_MAPPING);
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
   public int getCategoryId()
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
      fieldSet.add(CATEGORY_ID);
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
      fieldSet.add(GEOLOCATION_CTRL_MODE);
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
      fieldSet.add(GEO_AREAS);
   }
}
