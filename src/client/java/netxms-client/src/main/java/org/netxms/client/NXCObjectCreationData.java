/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.client.constants.SensorDeviceClass;
import org.netxms.client.maps.MapType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkService;

/**
 * NetXMS object creation data
 */
public class NXCObjectCreationData
{
	// Creation flags
	public static int CF_DISABLE_ICMP         = 0x0001;
	public static int CF_DISABLE_NXCP         = 0x0002;
	public static int CF_DISABLE_SNMP         = 0x0004;
	public static int CF_CREATE_UNMANAGED     = 0x0008;
   public static int CF_ENTER_MAINTENANCE    = 0x0010;
   public static int CF_AS_ZONE_PROXY        = 0x0020;
   public static int CF_DISABLE_ETHERNET_IP  = 0x0040;
   public static int CF_SNMP_SETTINGS_LOCKED = 0x0080;
   public static int CF_EXTERNAL_GATEWAY     = 0x0100;
   public static int CF_DISABLE_SSH          = 0x0200;
   public static int CF_DISABLE_MODBUS_TCP   = 0x0400;
   public static int CF_DISABLE_VNC          = 0x0800;

	private int objectClass;
	private String name;
   private String alias;
	private long parentId;
	private String comments;
	private int creationFlags;
	private String primaryName;
	private int agentPort;
	private int snmpPort;
   private int etherNetIpPort;
   private int modbusTcpPort;
   private short modbusUnitId;
   private int sshPort;
   private int vncPort;
	private InetAddressEx ipAddress;
	private long agentProxyId;
	private long snmpProxyId;
   private long mqttProxyId;
   private long etherNetIpProxyId;
   private long modbusProxyId;
   private long icmpProxyId;
   private long sshProxyId;
   private long vncProxyId;
   private long webServiceProxyId;
   private MapType mapType;
	private List<Long> seedObjectIds;
	private int zoneUIN;
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
   private int chassis;
	private int module;
	private int pic;
	private int port;
	private boolean physicalPort;
	private boolean createStatusDci;
	private String deviceId;
	private int height;
	private int flags;
	private long controllerId;
	private long chassisId;
	private String sshLogin;
	private String sshPassword;
   private String vncPassword;
   private SensorDeviceClass deviceClass;
   private String vendor;
   private String model;
   private String serialNumber;
   private String deviceAddress;
   private long gatewayNodeId;
   private int instanceDiscoveryMethod;
   private long assetId;
   private Map<String, String> assetProperties;

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
			ipAddress = new InetAddressEx(InetAddress.getByName("127.0.0.1"), 8);
		}
		catch(UnknownHostException e)
		{
		}

		primaryName = null;
      alias = null;
      agentPort = 4700;
      snmpPort = 161;
      etherNetIpPort = 44818;
      modbusTcpPort = 502;
      modbusUnitId = 1;
      sshPort = 22;
      vncPort = 5900;
		comments = null;
		creationFlags = 0;
		agentProxyId = 0;
		snmpProxyId = 0;
      mqttProxyId = 0;
      etherNetIpProxyId = 0;
      modbusProxyId = 0;
		icmpProxyId = 0;
		sshProxyId = 0;
      vncProxyId = 0;
		mapType = MapType.CUSTOM;
		seedObjectIds = new ArrayList<Long>();
		zoneUIN = 0;
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
		chassis = 0;
		module = 0;
		pic = 0;
		port = 0;
		physicalPort = false;
		createStatusDci = false;
		sshLogin = "";
		sshPassword = "";
      vncPassword = "";
      deviceClass = SensorDeviceClass.OTHER;
	   vendor = "";
      model = "";
	   serialNumber = "";
	   deviceAddress = "";
	   gatewayNodeId = 0;
	   webServiceProxyId = 0;
	}
	
	/**
	 * Update creation data from modify object
	 * 
	 * @param data modifyt data
	 */
	public void updateFromModificationData(NXCObjectModificationData data)
	{
      if (data.getPrimaryName() != null)
         primaryName = data.getPrimaryName();
      if (data.getAlias() != null)
         alias = data.getAlias();
      if (data.getAgentPort() != null)
         agentPort = data.getAgentPort();
      if (data.getSnmpPort() != null)
         snmpPort = data.getSnmpPort();
      if (data.getEtherNetIPPort() != null)
         etherNetIpPort = data.getEtherNetIPPort();
      if (data.getModbusTcpPort() != null)
         modbusTcpPort = data.getModbusTcpPort();
      if (data.getModbusUnitId() != null)
         modbusUnitId = data.getModbusUnitId();
      if (data.getSshPort() != null)
         sshPort = data.getSshPort();
      if (data.getVncPort() != null)
         vncPort = data.getVncPort();
      if (data.getIpAddress() != null)
         ipAddress = data.getIpAddress();
      if (data.getAgentProxy() != null)
         agentProxyId = data.getAgentProxy();
      if (data.getSnmpProxy() != null)
         snmpProxyId = data.getSnmpProxy();
      if (data.getMqttProxy() != null)
         mqttProxyId = data.getMqttProxy();
      if (data.getEtherNetIPProxy() != null)
         etherNetIpProxyId = data.getEtherNetIPProxy();
      if (data.getModbusProxy() != null)
         modbusProxyId = data.getModbusProxy();
      if (data.getIcmpProxy() != null)
         icmpProxyId = data.getIcmpProxy();
      if (data.getSshProxy() != null)
         sshProxyId = data.getSshProxy();
      if (data.getVncProxy() != null)
         vncProxyId = data.getVncProxy();
      if (data.getSeedObjectIdsAsList() != null)
         seedObjectIds = data.getSeedObjectIdsAsList();
      if (data.getServiceType() != null)
         serviceType = data.getServiceType();
      if (data.getIpProtocol() != null)
         ipProtocol = data.getIpProtocol();
      if (data.getIpPort() != null)
         ipPort = data.getIpPort();
      if (data.getRequest() != null)
         request = data.getRequest();
      if (data.getResponse() != null)
         response = data.getResponse();
      if (data.getMacAddress() != null)
         macAddress = data.getMacAddress();
      if (data.getHeight() != null)
         height = data.getHeight();
      if (data.getControllerId() != null)
         controllerId = data.getControllerId();
      if (data.getChassisId() != null)
         chassisId = data.getChassisId();
      if (data.getSshLogin() != null)
         sshLogin = data.getSshLogin();
      if (data.getSshPassword() != null)
         sshPassword = data.getSshPassword();
      if (data.getVncPassword() != null)
         vncPassword = data.getVncPassword();
      if (data.getDeviceClass() != null)
         deviceClass = data.getDeviceClass();
      if (data.getVendor() != null)
         vendor = data.getVendor();
      if (data.getModel() != null)
         model = data.getModel();
      if (data.getSerialNumber() != null)
         serialNumber = data.getSerialNumber();
      if (data.getDeviceAddress() != null)
         deviceAddress = data.getDeviceAddress();
      if (data.getGatewayNodeId() != null)
         gatewayNodeId = data.getGatewayNodeId();
      if (data.getWebServiceProxy() != null)
         webServiceProxyId = data.getWebServiceProxy();
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
    * @return the mqttProxyId
    */
   public long getMqttProxyId()
   {
      return mqttProxyId;
   }

   /**
    * @param mqttProxyId the mqttProxyId to set
    */
   public void setMqttProxyId(long mqttProxyId)
   {
      this.mqttProxyId = mqttProxyId;
   }

   /**
    * @return the icmpProxyId
    */
   public long getIcmpProxyId()
   {
      return icmpProxyId;
   }

   /**
    * @param icmpProxyId the icmpProxyId to set
    */
   public void setIcmpProxyId(long icmpProxyId)
   {
      this.icmpProxyId = icmpProxyId;
   }

   /**
    * @return the sshProxyId
    */
   public long getSshProxyId()
   {
      return sshProxyId;
   }

   /**
    * @param sshProxyId the sshProxyId to set
    */
   public void setSshProxyId(long sshProxyId)
   {
      this.sshProxyId = sshProxyId;
   }

   /**
    * @return the vncProxyId
    */
   public long getVncProxyId()
   {
      return vncProxyId;
   }

   /**
    * @param vncProxyId the vncProxyId to set
    */
   public void setVncProxyId(long vncProxyId)
   {
      this.vncProxyId = vncProxyId;
   }

   /**
    * @return the webServiceProxyId
    */
   public long getWebServiceProxyId()
   {
      return webServiceProxyId;
   }

   /**
    * @param webServiceProxyId the webServiceProxyId to set
    */
   public void setWebServiceProxyId(long webServiceProxyId)
   {
      this.webServiceProxyId = webServiceProxyId;
   }

   /**
    * @return the mapType
    */
   public MapType getMapType()
	{
		return mapType;
	}

	/**
	 * @param mapType the mapType to set
	 */
   public void setMapType(MapType mapType)
	{
		this.mapType = mapType;
	}

	/**
	 * @return the seedObjectIds
	 */
	public Long[] getSeedObjectIds()
	{
		return seedObjectIds.toArray(new Long[seedObjectIds.size()]);
	}

	/**
	 * @param seedObjectId the seedObjectId to set
	 */
	public void setSeedObjectId(long seedObjectId)
	{
		seedObjectIds.clear();
		seedObjectIds.add(seedObjectId);
	}
	
	/**
	 * @param seedObjectIds the seed node object Ids to set
	 */
   public void setSeedObjectIds(Collection<Long> seedObjectIds)
	{
      this.seedObjectIds = new ArrayList<Long>(seedObjectIds);
	}

	/**
	 * @return the zoneId
	 */
	public int getZoneUIN()
	{
		return zoneUIN;
	}

	/**
	 * @param zoneUIN the zoneId to set
	 */
	public void setZoneUIN(int zoneUIN)
	{
		this.zoneUIN = zoneUIN;
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
    * @param alias the alias to set
    */
   public void setObjectAlias(String alias)
   {
      this.alias = alias;
   }

   /**
    * @return the alias
    */
   public String getObjectAlias()
   {
      return alias;
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
	 * @return interface module number
	 */
	public int getModule()
	{
		return module;
	}

	/**
	 * @param module interface module number
	 */
	public void setModule(int module)
	{
		this.module = module;
	}

	/**
    * @return the chassis
    */
   public int getChassis()
   {
      return chassis;
   }

   /**
    * @param chassis the chassis to set
    */
   public void setChassis(int chassis)
   {
      this.chassis = chassis;
   }

   /**
    * @return the pic
    */
   public int getPIC()
   {
      return pic;
   }

   /**
    * @param pic the pic to set
    */
   public void setPIC(int pic)
   {
      this.pic = pic;
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
	}

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * @param flags the flags to set
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
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
    * @param serialNumber the serialNumber to set
    */
   public void setSerialNumber(String serialNumber)
   {
      this.serialNumber = serialNumber;
   }
   
   /**
    * @return the vendor
    */
   public final String getVendor()
   {
      return vendor;
   }

   /**
    * @return the serialNumber
    */
   public final String getSerialNumber()
   {
      return serialNumber;
   }

   /**
    * @return the sensorProxy
    */
   public long getGatewayNodeId()
   {
      return gatewayNodeId;
   }

   /**
    * @param gatewayNodeId the sensorProxy to set
    */
   public void setGatewayNodeId(long gatewayNodeId)
   {
      this.gatewayNodeId = gatewayNodeId;
   }

   /**
    * @return the etherNetIpPort
    */
   public int getEtherNetIpPort()
   {
      return etherNetIpPort;
   }

   /**
    * @param etherNetIpPort the etherNetIpPort to set
    */
   public void setEtherNetIpPort(int etherNetIpPort)
   {
      this.etherNetIpPort = etherNetIpPort;
   }

   /**
    * @return the etherNetIpProxyId
    */
   public long getEtherNetIpProxyId()
   {
      return etherNetIpProxyId;
   }

   /**
    * @param etherNetIpProxyId the etherNetIpProxyId to set
    */
   public void setEtherNetIpProxyId(long etherNetIpProxyId)
   {
      this.etherNetIpProxyId = etherNetIpProxyId;
   }

   /**
    * @return the modbusTcpPort
    */
   public int getModbusTcpPort()
   {
      return modbusTcpPort;
   }

   /**
    * @param modbusTcpPort the modbusTcpPort to set
    */
   public void setModbusTcpPort(int modbusTcpPort)
   {
      this.modbusTcpPort = modbusTcpPort;
   }

   /**
    * @return the modbusUnitId
    */
   public short getModbusUnitId()
   {
      return modbusUnitId;
   }

   /**
    * @param modbusUnitId the modbusUnitId to set
    */
   public void setModbusUnitId(short modbusUnitId)
   {
      this.modbusUnitId = modbusUnitId;
   }

   /**
    * @return the modbusProxyId
    */
   public long getModbusProxyId()
   {
      return modbusProxyId;
   }

   /**
    * @param modbusProxyId the modbusProxyId to set
    */
   public void setModbusProxyId(long modbusProxyId)
   {
      this.modbusProxyId = modbusProxyId;
   }

   /**
    * @return the sshPort
    */
   public int getSshPort()
   {
      return sshPort;
   }

   /**
    * @param sshPort the sshPort to set
    */
   public void setSshPort(int sshPort)
   {
      this.sshPort = sshPort;
   }

   /**
    * @return the vncPort
    */
   public int getVncPort()
   {
      return vncPort;
   }

   /**
    * @param vncPort the vncPort to set
    */
   public void setVncPort(int vncPort)
   {
      this.vncPort = vncPort;
   }

   /**
    * @return the instanceDiscoveryMethod
    */
   public int getInstanceDiscoveryMethod()
   {
      return instanceDiscoveryMethod;
   }

   /**
    * @param instanceDiscoveryMethod the instanceDiscoveryMethod to set
    */
   public void setInstanceDiscoveryMethod(int instanceDiscoveryMethod)
   {
      this.instanceDiscoveryMethod = instanceDiscoveryMethod;
   }

   /**
    * @return the assetId
    */
   public long getAssetId()
   {
      return assetId;
   }

   /**
    * @param assetId the assetId to set
    */
   public void setAssetId(long assetId)
   {
      this.assetId = assetId;
   }

   /**
    * @return the assetProperties
    */
   public Map<String, String> getAssetProperties()
   {
      return assetProperties;
   }

   /**
    * @param assetProperties the assetProperties to set
    */
   public void setAssetProperties(Map<String, String> assetProperties)
   {
      this.assetProperties = assetProperties;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "NXCObjectCreationData [objectClass=" + objectClass + ", name=" + name + ", alias=" + alias + ", parentId=" + parentId + ", comments=" + comments + ", creationFlags=" + creationFlags +
            ", primaryName=" + primaryName + ", agentPort=" + agentPort + ", snmpPort=" + snmpPort + ", etherNetIpPort=" + etherNetIpPort + ", modbusTcpPort=" + modbusTcpPort + ", modbusUnitId=" +
            modbusUnitId + ", sshPort=" + sshPort + ", ipAddress=" + ipAddress + ", agentProxyId=" + agentProxyId + ", snmpProxyId=" + snmpProxyId + ", mqttProxyId=" + mqttProxyId +
            ", etherNetIpProxyId=" + etherNetIpProxyId + ", modbusProxyId=" + modbusProxyId + ", icmpProxyId=" + icmpProxyId + ", sshProxyId=" + sshProxyId + ", webServiceProxyId=" +
            webServiceProxyId + ", mapType=" + mapType + ", seedObjectIds=" + seedObjectIds + ", zoneUIN=" + zoneUIN + ", serviceType=" + serviceType + ", ipProtocol=" + ipProtocol + ", ipPort=" +
            ipPort + ", request=" + request + ", response=" + response + ", linkedNodeId=" + linkedNodeId + ", template=" + template + ", macAddress=" + macAddress + ", ifIndex=" + ifIndex +
            ", ifType=" + ifType + ", chassis=" + chassis + ", module=" + module + ", pic=" + pic + ", port=" + port + ", physicalPort=" + physicalPort + ", createStatusDci=" + createStatusDci +
            ", deviceId=" + deviceId + ", height=" + height + ", flags=" + flags + ", controllerId=" + controllerId + ", chassisId=" + chassisId + ", sshLogin=" + sshLogin + ", sshPassword=" +
            sshPassword + ", deviceClass=" + deviceClass + ", vendor=" + vendor + ", model=" + model + ", serialNumber=" + serialNumber + ", deviceAddress=" + deviceAddress + ", gatewayNodeId=" +
            gatewayNodeId + ", instanceDiscoveryMethod=" + instanceDiscoveryMethod + ", assetId=" + assetId + ", assetProperties=" + assetProperties + "]";
   }
}
