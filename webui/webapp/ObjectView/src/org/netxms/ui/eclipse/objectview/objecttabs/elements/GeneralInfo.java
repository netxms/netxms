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
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.swt.widgets.Composite;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.SpanningTreePortState;
import org.netxms.client.maps.MapType;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.BusinessService;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.NetworkMap;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Template;
import org.netxms.client.objects.Zone;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfaceListLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * "General" element on object overview tab
 */
public class GeneralInfo extends TableElement
{
   private final String[] ifaceExpectedState = { Messages.get().InterfaceListLabelProvider_StateUp, Messages.get().InterfaceListLabelProvider_StateDown, Messages.get().InterfaceListLabelProvider_StateIgnore, Messages.get().GeneralInfo_Auto };
   
	/**
	 * @param parent
	 * @param anchor
	 * @param objectTab
	 */
	public GeneralInfo(Composite parent, OverviewPageElement anchor, ObjectTab objectTab)
	{
		super(parent, anchor, objectTab);
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.TableElement#fillTable()
    */
	@Override
	protected void fillTable()
	{
		final AbstractObject object = getObject();
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		
		addPair(Messages.get().GeneralInfo_ID, Long.toString(object.getObjectId()));
		if (object.getGuid() != null)
			addPair(Messages.get().GeneralInfo_GUID, object.getGuid().toString());
		addPair(Messages.get().GeneralInfo_Class, object.getObjectClassName());
      addPair(Messages.get().GeneralInfo_Alias, object.getAlias(), false);
      if (object.isInMaintenanceMode())
      {
         addPair(Messages.get().GeneralInfo_Status, StatusDisplayInfo.getStatusText(object.getStatus()) + Messages.get().GeneralInfo_Maintenance);
         AbstractUserObject user = session.findUserDBObjectById(object.getMaintenanceInitiatorId(), new Runnable() {
            @Override
            public void run()
            {
               getDisplay().asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     onObjectChange(); // will cause refresh of table content
                  }
               });
            }
         });
         addPair("Maintenance initiator", (user != null) ? user.getName() : "[" + object.getMaintenanceInitiatorId() + "]");
      }
		else
      {
         addPair(Messages.get().GeneralInfo_Status, StatusDisplayInfo.getStatusText(object.getStatus()));
      }
      if ((object instanceof AbstractNode) && ((((AbstractNode)object).getCapabilities() & AbstractNode.NC_IS_ETHERNET_IP) != 0))
      {
         addPair("Device state", ((AbstractNode)object).getCipStateText(), false);
         addPair("Device status", ((AbstractNode)object).getCipStatusText(), false);
         if (((((AbstractNode)object).getCipStatus() >> 4) & 0x0F) != 0)
            addPair("Extended device status", ((AbstractNode)object).getCipExtendedStatusText(), false);
      }
      if (object.getCreationTime() != null && object.getCreationTime().getTime() != 0)
         addPair("Creation time", RegionalSettings.getDateTimeFormat().format(object.getCreationTime()), false);
		switch(object.getObjectClass())
		{
         case AbstractObject.OBJECT_CHASSIS:
            Chassis chassis = (Chassis)object;
            if (chassis.getControllerId() != 0)
            {
               AbstractNode node = session.findObjectById(chassis.getControllerId(), AbstractNode.class);
               if (node != null)
               {
                  addPair(Messages.get().GeneralInfo_Controller, node.getObjectName());
               }
            }
            if (chassis.getPhysicalContainerId() != 0)
            {
               Rack rack = session.findObjectById(chassis.getPhysicalContainerId(), Rack.class);
               if (rack != null)
               {
                  addPair(Messages.get().GeneralInfo_Rack, String.format(Messages.get().GeneralInfo_Units, rack.getObjectName(),
                          rack.isTopBottomNumbering() ? chassis.getRackPosition() : chassis.getRackPosition() - chassis.getRackHeight() + 1,
                          rack.isTopBottomNumbering() ? chassis.getRackPosition() + chassis.getRackHeight() - 1 : chassis.getRackPosition()));
               }
            }
            break;
         case AbstractObject.OBJECT_INTERFACE:
            Interface iface = (Interface)object;
            Interface parentIface = iface.getParentInterface();
            if (parentIface != null)
               addPair("Parent interface", parentIface.getObjectName());
            addPair(Messages.get().GeneralInfo_IfIndex, Integer.toString(iface.getIfIndex()));
				String typeName = iface.getIfTypeName();
				addPair(Messages.get().GeneralInfo_IfType, (typeName != null) ? String.format("%d (%s)", iface.getIfType(), typeName) : Integer.toString(iface.getIfType())); //$NON-NLS-1$
				addPair(Messages.get().GeneralInfo_Description, iface.getDescription(), false);
            addPair("Interface alias", iface.getIfAlias(), false);
            if (iface.getMtu() > 0)
               addPair(Messages.get().GeneralInfo_MTU, Integer.toString(iface.getMtu()));
            if (iface.getSpeed() > 0)
               addPair(Messages.get().GeneralInfo_Speed, InterfaceListLabelProvider.ifSpeedTotext(iface.getSpeed()));
            addPair(Messages.get().GeneralInfo_MACAddr, iface.getMacAddress().toString());
            String vendor = session.getVendorByMac(iface.getMacAddress(), null);
            if (vendor != null && !vendor.isEmpty())
               addPair("MAC Address Vendor", vendor);               
				if (iface.isPhysicalPort())
				{
					addPair(Messages.get().GeneralInfo_SlotPort, iface.getPhysicalLocation());
					AbstractNode node = iface.getParentNode();
               if (node != null)
               {
                  if (node.isBridge() && (iface.getStpPortState() != SpanningTreePortState.UNKNOWN))
                  {
                     addPair("Spanning Tree port state", iface.getStpPortState().getText());
                  }
                  if (node.is8021xSupported())
                  {
                     addPair(Messages.get().GeneralInfo_8021xPAE, iface.getDot1xPaeStateAsText());
                     addPair(Messages.get().GeneralInfo_8021xBackend, iface.getDot1xBackendStateAsText());
                  }
               }
				}
            addPair("VLAN", InterfaceListLabelProvider.getVlanList(iface), false);
				if (iface.getIpAddressList().size() > 0)
				{
					if (session.isZoningEnabled())
						addPair(Messages.get().GeneralInfo_ZoneId, getZoneName(iface.getZoneId()));
					addPair(Messages.get().GeneralInfo_IPAddr, iface.getIpAddressList().get(0).toString());
					for(int i = 1; i < iface.getIpAddressList().size(); i++)
	               addPair("", iface.getIpAddressList().get(i).toString()); //$NON-NLS-1$
				}
            if (iface.isOSPF())
            {
               addPair("OSPF area", iface.getOSPFArea().getHostAddress());
               addPair("OSPF interface type", iface.getOSPFType().getText(), false);
               addPair("OSPF interface state", iface.getOSPFState().getText(), false);
            }
            addPair(Messages.get().GeneralInfo_AdmState, iface.getAdminStateAsText());
            addPair(Messages.get().GeneralInfo_OperState, iface.getOperStateAsText());
            addPair(Messages.get().GeneralInfo_ExpectedState, ifaceExpectedState[iface.getExpectedState()]);
				break;
			case AbstractObject.OBJECT_NODE:
				AbstractNode node = (AbstractNode)object;
				if (session.isZoningEnabled())
					addPair(Messages.get().GeneralInfo_ZoneId, getZoneName(node.getZoneId()));
            if ((node.getFlags() & AbstractNode.NF_EXTERNAL_GATEWAY) != 0)
            {
               addPair(Messages.get().GeneralInfo_PrimaryHostName, node.getPrimaryName() + " (external gateway)");
               addPair(Messages.get().GeneralInfo_PrimaryIP, node.getPrimaryIP().getHostAddress() + " (external gateway)");
            }
            else
            {
               addPair(Messages.get().GeneralInfo_PrimaryHostName, node.getPrimaryName());
               addPair(Messages.get().GeneralInfo_PrimaryIP, node.getPrimaryIP().getHostAddress());
            }
            addPair(Messages.get().GeneralInfo_NodeType, node.getNodeType().toString(), false);
            addPair(Messages.get().GeneralInfo_HypervisorType, node.getHypervisorType(), false);
            addPair(Messages.get().GeneralInfo_HypervisorInformation, node.getHypervisorInformation(), false);
            addPair(Messages.get().SensorStatus_Vendor, node.getHardwareVendor(), false);
            if ((node.getCapabilities() & AbstractNode.NC_IS_ETHERNET_IP) != 0)
               addPair("Device type", node.getCipDeviceTypeName(), false);
            addPair("Product name", node.getHardwareProductName(), false);
            addPair("Product code", node.getHardwareProductCode(), false);
            addPair("Product version", node.getHardwareProductVersion(), false);
            addPair(Messages.get().SensorStatus_SerialNumber, node.getHardwareSerialNumber(), false);
				if (node.hasAgent())
				{
               addPair("Hardware ID", node.getHardwareIdAsText(), false);
					addPair(Messages.get().GeneralInfo_AgentVersion, node.getAgentVersion());
					if (node.getAgentId() != null)
					   addPair(Messages.get().GeneralInfo_AgentId, node.getAgentId().toString());
				}
				addPair(Messages.get().GeneralInfo_SysDescr, node.getSystemDescription(), false);
				addPair(Messages.get().GeneralInfo_PlatformName, node.getPlatformName(), false);
				addPair(Messages.get().GeneralInfo_SysName, node.getSnmpSysName(), false);
				addPair(Messages.get().GeneralInfo_SysOID, node.getSnmpOID(), false);
            addPair(Messages.get().GeneralInfo_SNMPsysLocation, node.getSnmpSysLocation(), false);
            addPair(Messages.get().GeneralInfo_SNMPsysContact, node.getSnmpSysContact(), false);
            if (node.isBridge())
					addPair(Messages.get().GeneralInfo_BridgeBaseAddress, node.getBridgeBaseAddress().toString());
            if (node.isOSPF())
               addPair("OSPF router ID", node.getOSPFRouterId().getHostAddress());
				addPair(Messages.get().GeneralInfo_Driver, node.getDriverName(), false);
            if (node.getBootTime() != null)
               addPair(Messages.get().GeneralInfo_BootTime, RegionalSettings.getDateTimeFormat().format(node.getBootTime()), false);
            if (node.hasAgent())
               addPair(Messages.get().GeneralInfo_AgentStatus, (node.getStateFlags() & Node.NSF_AGENT_UNREACHABLE) != 0 ? Messages.get().GeneralInfo_Unreachable : Messages.get().GeneralInfo_Connected);
            if (node.getLastAgentCommTime() != null)
               addPair(Messages.get().GeneralInfo_LastAgentContact, RegionalSettings.getDateTimeFormat().format(node.getLastAgentCommTime()), false);
            if (node.getPhysicalContainerId() != 0)
            {
               Rack rack = session.findObjectById(node.getPhysicalContainerId(), Rack.class);
               if (rack != null)
               {
                  addPair(Messages.get().GeneralInfo_Rack, String.format(Messages.get().GeneralInfo_Units, rack.getObjectName(),
                          rack.isTopBottomNumbering() ? node.getRackPosition() : node.getRackPosition() - node.getRackHeight() + 1,
                          rack.isTopBottomNumbering() ? node.getRackPosition() + node.getRackHeight() - 1 : node.getRackPosition()));
               }
            }
            if (node.isIcmpStatisticsCollected())
            {
               addPair("ICMP average response time", node.getIcmpAverageResponseTime() + " ms");
               addPair("ICMP packet loss", node.getIcmpPacketLoss() + "%");
            }
				break;
			case AbstractObject.OBJECT_MOBILEDEVICE:
				MobileDevice md = (MobileDevice)object;
				if (md.getLastReportTime().getTime() == 0)
					addPair(Messages.get().GeneralInfo_LastReport, Messages.get().GeneralInfo_Never);
				else
					addPair(Messages.get().GeneralInfo_LastReport, RegionalSettings.getDateTimeFormat().format(md.getLastReportTime()));
				addPair(Messages.get().GeneralInfo_DeviceId, md.getDeviceId());
            addPair("Communication protocol", md.getCommProtocol());
				addPair(Messages.get().GeneralInfo_Vendor, md.getVendor());
				addPair(Messages.get().GeneralInfo_Model, md.getModel());
				addPair(Messages.get().GeneralInfo_Serial, md.getSerialNumber());
				addPair(Messages.get().GeneralInfo_OS, md.getOsName());
				addPair(Messages.get().GeneralInfo_OSVersion, md.getOsVersion());
				addPair(Messages.get().GeneralInfo_User, md.getUserId(), false);
				if (md.getBatteryLevel() >= 0)
					addPair(Messages.get().GeneralInfo_BatteryLevel, Integer.toString(md.getBatteryLevel()) + "%"); //$NON-NLS-1$
				break;
         case AbstractObject.OBJECT_SENSOR:
            Sensor sensor = (Sensor)object;
            addPair(Messages.get().SensorStatus_DeviceAddress, sensor.getDeviceAddress(), false);
            if (sensor.getMacAddress() != null && !sensor.getMacAddress().isNull())
            {
               addPair(Messages.get().SensorStatus_MacAddress, sensor.getMacAddress().toString(), true);
               String vendorMac = session.getVendorByMac(sensor.getMacAddress(), null);
               if (vendorMac != null && !vendorMac.isEmpty())
                  addPair("MAC Address Vendor", vendorMac);   
            }
            addPair(Messages.get().SensorStatus_Vendor, sensor.getVendor(), true);            
            addPair(Messages.get().SensorStatus_DeviceClass, sensor.getDeviceClass().getDisplayName());
            addPair(Messages.get().SensorStatus_SerialNumber, sensor.getSerialNumber(), true);
            break;
			case AbstractObject.OBJECT_ACCESSPOINT:
				AccessPoint ap = (AccessPoint)object;
            addPair(Messages.get().GeneralInfo_State, ap.getState().toString());
				addPair(Messages.get().GeneralInfo_Vendor, ap.getVendor());
				addPair(Messages.get().GeneralInfo_Model, ap.getModel());
				addPair(Messages.get().GeneralInfo_Serial, ap.getSerialNumber());
				addPair(Messages.get().GeneralInfo_MACAddr, ap.getMacAddress().toString());
            String vendorMac = session.getVendorByMac(ap.getMacAddress(), null);
            if (vendorMac != null && !vendorMac.isEmpty())
               addPair("MAC Address Vendor", vendorMac);   
				if (ap.getIpAddress().isValidAddress())
				   addPair(Messages.get().GeneralInfo_IPAddr, ap.getIpAddress().getHostAddress());
				break;
			case AbstractObject.OBJECT_SUBNET:
				Subnet subnet = (Subnet)object;
				if (session.isZoningEnabled())
					addPair(Messages.get().GeneralInfo_ZoneId, getZoneName(subnet.getZoneId()));
            addPair(Messages.get().GeneralInfo_IPAddress, subnet.getNetworkAddress().toString());
				break;
         case AbstractObject.OBJECT_TEMPLATE:
            Template template = (Template)object;
            addPair("Number of DCIs", Integer.toString(template.getNumDataCollectionItems()));
            addPair("Number of policies", Integer.toString(template.getNumPolicies()));
            break;
			case AbstractObject.OBJECT_ZONE:
				Zone zone = (Zone)object;
				addPair(Messages.get().GeneralInfo_ZoneId, Long.toString(zone.getUIN()));
				break;
         case AbstractObject.OBJECT_BUSINESSSERVICE:
            BusinessService businessService = (BusinessService)object;
            addPair("Service state", businessService.getServiceState().toString());
            break;
         case AbstractObject.OBJECT_CLUSTER:
            Cluster cluster = (Cluster)object;
            if (session.isZoningEnabled())
               addPair("Zone UIN", getZoneName(cluster.getZoneId()));
            break;
         case AbstractObject.OBJECT_NETWORKMAP:
            NetworkMap map = (NetworkMap)object;
            addPair("Map type", getMapTypeDescription(map.getMapType()));
            break;
			default:
				break;
		}
      if (object instanceof DataCollectionTarget)
      {
         addPair("Number of DCIs", Integer.toString(((DataCollectionTarget)object).getNumDataCollectionItems()));
      }
		if (object.getGeolocation().getType() != GeoLocation.UNSET)
      {
         addPair(Messages.get().GeneralInfo_Location, object.getGeolocation().toString());
         if (object instanceof MobileDevice)
         {
            MobileDevice md = (MobileDevice)object;
            if (md.getSpeed() >= 0)
               addPair(Messages.get().GeneralInfo_Speed, Double.toString(md.getSpeed()) + " km/h");
            if (md.getDirection() >= 0)
               addPair("Direction", Integer.toString(md.getDirection()) + "\u00b0");
            addPair("Altitude", Integer.toString(md.getAltitude()) + " m");
         }
      }
      if (!object.getPostalAddress().isEmpty())
         addPair(Messages.get().GeneralInfo_PostalAddress, object.getPostalAddress().getAddressLine());
	}

	/**
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return Messages.get().GeneralInfo_Title;
	}
	
	/**
	 * Get zone name
	 * 
	 * @param zoneId
	 * @return
	 */
	private String getZoneName(int zoneId)
	{
	   Zone zone = ConsoleSharedData.getSession().findZone(zoneId);
	   if (zone == null)
	      return Long.toString(zoneId);
	   return String.format("%d (%s)", zoneId, zone.getObjectName());
	}

   /**
    * Get textual description for given map type.
    *
    * @param mapType map type code
    * @return textual description for given map type
    */
   private String getMapTypeDescription(MapType mapType)
   {
      switch(mapType)
      {
         case CUSTOM:
            return "Custom";
         case HYBRID_TOPOLOGY:
            return "Hybrid Topology";
         case INTERNAL_TOPOLOGY:
            return "Internal Topology";
         case IP_TOPOLOGY:
            return "IP Topology";
         case LAYER2_TOPOLOGY:
            return "L2 Topology";
         case OSPF_TOPOLOGY:
            return "OSPF Topology";
      }
      return String.format("Unknown (%d)", mapType.getValue());
   }
}
