/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.elements;

import java.text.NumberFormat;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.NodeLink;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.ServiceCheck;
import org.netxms.client.objects.ServiceContainer;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.views.helpers.InterfaceListLabelProvider;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * "General" element on object overview tab
 */
public class GeneralInfo extends TableElement
{
   private static final I18n i18n = LocalizationHelper.getI18n(GeneralInfo.class);
   private static final String[] ifaceExpectedState = { i18n.tr("Up"), i18n.tr("Down"), i18n.tr("Ignore"), i18n.tr("Auto") };
   
	/**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public GeneralInfo(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
      super(parent, anchor, objectView);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#fillTable()
    */
	@Override
	protected void fillTable()
	{
		final AbstractObject object = getObject();
      final NXCSession session = Registry.getSession();
		
      addPair(i18n.tr("ID"), Long.toString(object.getObjectId()));
		if (object.getGuid() != null)
         addPair(i18n.tr("GUID"), object.getGuid().toString());
      addPair(i18n.tr("Class"), object.getObjectClassName());
		if (object.isInMaintenanceMode())
      {
         addPair(i18n.tr("Status"), StatusDisplayInfo.getStatusText(object.getStatus()) + i18n.tr(" (maintenance)"));
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
         addPair(i18n.tr("Maintenance initiator"),
               (user != null) ? user.getName() : "[" + object.getMaintenanceInitiatorId() + "]");
      }
		else
      {
         addPair(i18n.tr("Status"), StatusDisplayInfo.getStatusText(object.getStatus()));
      }
      if ((object instanceof AbstractNode) && ((((AbstractNode)object).getCapabilities() & AbstractNode.NC_IS_ETHERNET_IP) != 0))
      {
         addPair(i18n.tr("Device state"), ((AbstractNode)object).getCipStateText(), false);
         addPair(i18n.tr("Device status"), ((AbstractNode)object).getCipStatusText(), false);
         if (((((AbstractNode)object).getCipStatus() >> 4) & 0x0F) != 0)
            addPair(i18n.tr("Extended device status"), ((AbstractNode)object).getCipExtendedStatusText(), false);
      }
      if (object.getCreationTime() != null && object.getCreationTime().getTime() != 0)
         addPair(i18n.tr("Creation time"), DateFormatFactory.getDateTimeFormat().format(object.getCreationTime()), false);
		switch(object.getObjectClass())
		{
         case AbstractObject.OBJECT_CHASSIS:
            Chassis chassis = (Chassis)object;
            if (chassis.getControllerId() != 0)
            {
               AbstractNode node = session.findObjectById(chassis.getControllerId(), AbstractNode.class);
               if (node != null)
               {
                  addPair(i18n.tr("Controller"), node.getObjectName());
               }
            }
            if (chassis.getPhysicalContainerId() != 0)
            {
               Rack rack = session.findObjectById(chassis.getPhysicalContainerId(), Rack.class);
               if (rack != null)
               {
                  addPair(i18n.tr("Rack"), String.format(i18n.tr("%s (units %d-%d)"), rack.getObjectName(),
                          rack.isTopBottomNumbering() ? chassis.getRackPosition() : chassis.getRackPosition() - chassis.getRackHeight() + 1,
                          rack.isTopBottomNumbering() ? chassis.getRackPosition() + chassis.getRackHeight() - 1 : chassis.getRackPosition()));
               }
            }
            break;
			case AbstractObject.OBJECT_INTERFACE:
				Interface iface = (Interface)object;
				Interface parentIface = iface.getParentInterface();
				if (parentIface != null)
               addPair(i18n.tr("Parent interface"), parentIface.getObjectName());
            addPair(i18n.tr("Interface index"), Integer.toString(iface.getIfIndex()));
				String typeName = iface.getIfTypeName();
            addPair(i18n.tr("Interface type"),
                  (typeName != null) ? String.format("%d (%s)", iface.getIfType(), typeName) : Integer.toString(iface.getIfType()));
            addPair(i18n.tr("Description"), iface.getDescription(), false);
            addPair(i18n.tr("Alias"), iface.getAlias(), false);
            if (iface.getMtu() > 0)
               addPair(i18n.tr("MTU"), Integer.toString(iface.getMtu()));
            if (iface.getSpeed() > 0)
               addPair(i18n.tr("Speed"), InterfaceListLabelProvider.ifSpeedTotext(iface.getSpeed()));
            addPair(i18n.tr("MAC address"), iface.getMacAddress().toString());
				if (iface.isPhysicalPort())
				{
               addPair(i18n.tr("Physical location"), iface.getPhysicalLocation());
					AbstractNode node = iface.getParentNode();
					if ((node != null) && node.is8021xSupported())
					{
                  addPair(i18n.tr("802.1x PAE state"), iface.getDot1xPaeStateAsText());
                  addPair(i18n.tr("802.1x backend state"), iface.getDot1xBackendStateAsText());
					}
				}
				if (iface.getIpAddressList().size() > 0)
				{
					if (session.isZoningEnabled())
                  addPair(i18n.tr("Zone UIN"), getZoneName(iface.getZoneId()));
               addPair(i18n.tr("IP address"), iface.getIpAddressList().get(0).toString());
					for(int i = 1; i < iface.getIpAddressList().size(); i++)
	               addPair("", iface.getIpAddressList().get(i).toString()); //$NON-NLS-1$
				}
            addPair(i18n.tr("Administrative state"), iface.getAdminStateAsText());
            addPair(i18n.tr("Operational state"), iface.getOperStateAsText());
            addPair(i18n.tr("Expected state"), ifaceExpectedState[iface.getExpectedState()]);
				break;
			case AbstractObject.OBJECT_NODE:
				AbstractNode node = (AbstractNode)object;
				if (session.isZoningEnabled())
               addPair(i18n.tr("Zone UIN"), getZoneName(node.getZoneId()));
            addPair(i18n.tr("Primary host name"), node.getPrimaryName());
            addPair(i18n.tr("Primary IP address"), node.getPrimaryIP().getHostAddress());
            addPair(i18n.tr("Node type"), node.getNodeType().toString(), false);
            addPair(i18n.tr("Hypervisor type"), node.getHypervisorType(), false);
            addPair(i18n.tr("Hypervisor information"), node.getHypervisorInformation(), false);
            addPair(i18n.tr("Vendor"), node.getHardwareVendor(), false);
            if ((node.getCapabilities() & AbstractNode.NC_IS_ETHERNET_IP) != 0)
               addPair(i18n.tr("Device type"), node.getCipDeviceTypeName(), false);
            addPair(i18n.tr("Product name"), node.getHardwareProductName(), false);
            addPair(i18n.tr("Product code"), node.getHardwareProductCode(), false);
            addPair(i18n.tr("Product version"), node.getHardwareProductVersion(), false);
            addPair(i18n.tr("Serial number"), node.getHardwareSerialNumber(), false);
				if (node.hasAgent())
				{
               addPair(i18n.tr("Hardware ID"), node.getHardwareIdAsText(), false);
               addPair(i18n.tr("Agent version"), node.getAgentVersion());
					if (node.getAgentId() != null)
                  addPair(i18n.tr("Agent ID"), node.getAgentId().toString());
				}
            addPair(i18n.tr("System description"), node.getSystemDescription(), false);
            addPair(i18n.tr("Platform name"), node.getPlatformName(), false);
            addPair(i18n.tr("SNMP sysName"), node.getSnmpSysName(), false);
            addPair(i18n.tr("SNMP object ID"), node.getSnmpOID(), false);
            addPair(i18n.tr("SNMP sysLocation"), node.getSnmpSysLocation(), false);
            addPair(i18n.tr("SNMP sysContact"), node.getSnmpSysContact(), false);
				if ((node.getCapabilities() & AbstractNode.NC_IS_BRIDGE) != 0)
               addPair(i18n.tr("Bridge base address"), node.getBridgeBaseAddress().toString());
            addPair(i18n.tr("Driver"), node.getDriverName(), false);
            if (node.getBootTime() != null)
               addPair(i18n.tr("Boot time"), DateFormatFactory.getDateTimeFormat().format(node.getBootTime()), false);
            if (node.hasAgent())
               addPair(i18n.tr("Agent status"), (node.getStateFlags() & Node.NSF_AGENT_UNREACHABLE) != 0 ? i18n.tr("Unreachable") : i18n.tr("Connected"));
            if (node.getLastAgentCommTime() != null)
               addPair(i18n.tr("Last agent contact"), DateFormatFactory.getDateTimeFormat().format(node.getLastAgentCommTime()), false);
            if (node.getPhysicalContainerId() != 0)
            {
               Rack rack = session.findObjectById(node.getPhysicalContainerId(), Rack.class);
               if (rack != null)
               {
                  addPair(i18n.tr("Rack"), String.format(i18n.tr("%s (units %d-%d)"), rack.getObjectName(),
                          rack.isTopBottomNumbering() ? node.getRackPosition() : node.getRackPosition() - node.getRackHeight() + 1,
                          rack.isTopBottomNumbering() ? node.getRackPosition() + node.getRackHeight() - 1 : node.getRackPosition()));
               }
            }
            if (node.isIcmpStatisticsCollected())
            {
               addPair(i18n.tr("ICMP average response time"), node.getIcmpAverageResponseTime() + " ms");
               addPair(i18n.tr("ICMP packet loss"), node.getIcmpPacketLoss() + "%");
            }
				break;
			case AbstractObject.OBJECT_MOBILEDEVICE:
				MobileDevice md = (MobileDevice)object;
				if (md.getLastReportTime().getTime() == 0)
               addPair(i18n.tr("Last report"), i18n.tr("Never"));
				else
               addPair(i18n.tr("Last report"), DateFormatFactory.getDateTimeFormat().format(md.getLastReportTime()));
            addPair(i18n.tr("Device ID"), md.getDeviceId());
            addPair(i18n.tr("Vendor"), md.getVendor());
            addPair(i18n.tr("Model"), md.getModel());
            addPair(i18n.tr("Serial number"), md.getSerialNumber());
            addPair(i18n.tr("Operating System"), md.getOsName());
            addPair(i18n.tr("Operating System Version"), md.getOsVersion());
            addPair(i18n.tr("User"), md.getUserId(), false);
				if (md.getBatteryLevel() >= 0)
               addPair(i18n.tr("Battery level"), Integer.toString(md.getBatteryLevel()) + "%"); //$NON-NLS-1$
				break;
         case AbstractObject.OBJECT_SENSOR:
            Sensor sensor = (Sensor)object;
            addPair(i18n.tr("Device address"), sensor.getDeviceAddress(), false);
            if(sensor.getMacAddress() != null && !sensor.getMacAddress().isNull())
               addPair(i18n.tr("MAC address"), sensor.getMacAddress().toString(), true);
            addPair(i18n.tr("Vendor"), sensor.getVendor(), true);
            addPair(i18n.tr("Device class"), Sensor.DEV_CLASS_NAMES[sensor.getDeviceClass()]);
            addPair(i18n.tr("Communication protocol"), Sensor.COMM_METHOD[sensor.getCommProtocol()]);
            addPair(i18n.tr("Serial number"), sensor.getSerialNumber(), true);
            addPair(i18n.tr("Meta type"), sensor.getMetaType(), true);
            addPair(i18n.tr("Description"), sensor.getDescription(), true);
            if (sensor.getFrameCount() != 0)
               addPair(i18n.tr("Frame count"), Integer.toString(sensor.getFrameCount()));
            if (sensor.getSignalStrenght() != 1)
               addPair(i18n.tr("RSSI"), Integer.toString(sensor.getSignalStrenght()));
            if (sensor.getSignalNoise() != Integer.MAX_VALUE)
               addPair(i18n.tr("SNR"), Double.toString((double)sensor.getSignalNoise() / 10));
            if (sensor.getFrequency() != 0)
               addPair(i18n.tr("Frequency"), Double.toString((double)sensor.getFrequency() / 10));
            break;
			case AbstractObject.OBJECT_ACCESSPOINT:
				AccessPoint ap = (AccessPoint)object;
            addPair(i18n.tr("State"), ap.getState().toString());
            addPair(i18n.tr("Vendor"), ap.getVendor());
            addPair(i18n.tr("Model"), ap.getModel());
            addPair(i18n.tr("Serial number"), ap.getSerialNumber());
            addPair(i18n.tr("MAC address"), ap.getMacAddress().toString());
				if (ap.getIpAddress().isValidAddress())
               addPair(i18n.tr("IP address"), ap.getIpAddress().getHostAddress());
				break;
			case AbstractObject.OBJECT_SUBNET:
				Subnet subnet = (Subnet)object;
				if (session.isZoningEnabled())
               addPair(i18n.tr("Zone UIN"), getZoneName(subnet.getZoneId()));
            addPair(i18n.tr("IP address"), subnet.getNetworkAddress().toString());
				break;
			case AbstractObject.OBJECT_ZONE:
				Zone zone = (Zone)object;
            addPair(i18n.tr("Zone UIN"), Long.toString(zone.getUIN()));
				break;
			case AbstractObject.OBJECT_NODELINK:
				AbstractNode linkedNode = (AbstractNode)session.findObjectById(((NodeLink)object).getNodeId(), AbstractNode.class);
				if (linkedNode != null)
               addPair(i18n.tr("Linked node"), linkedNode.getObjectName());
			case AbstractObject.OBJECT_BUSINESSSERVICE:
			case AbstractObject.OBJECT_BUSINESSSERVICEROOT:
				ServiceContainer service = (ServiceContainer)object;
				NumberFormat nf = NumberFormat.getNumberInstance();
				nf.setMinimumFractionDigits(3);
				nf.setMaximumFractionDigits(3);
            addPair(i18n.tr("Uptime for day"), nf.format(service.getUptimeForDay()) + "%");
            addPair(i18n.tr("Uptime for week"), nf.format(service.getUptimeForWeek()) + "%");
            addPair(i18n.tr("Uptime for month"), nf.format(service.getUptimeForMonth()) + "%");
				break;
			case AbstractObject.OBJECT_SLMCHECK:
				ServiceCheck check = (ServiceCheck)object;
            addPair(i18n.tr("Is template"), check.isTemplate() ? i18n.tr("Yes") : i18n.tr("No"));
				if (check.getTemplateId() != 0)
				{
					ServiceCheck tmpl = (ServiceCheck)session.findObjectById(check.getTemplateId(), ServiceCheck.class);
					if (tmpl != null)
                  addPair(i18n.tr("Template"), tmpl.getObjectName());
				}
				break;
			default:
				break;
		}
		if (object.getGeolocation().getType() != GeoLocation.UNSET)
         addPair(i18n.tr("Location"), object.getGeolocation().toString());
		if (!object.getPostalAddress().isEmpty())
         addPair(i18n.tr("Postal Address"), object.getPostalAddress().getAddressLine());
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("General");
	}
	
	/**
	 * Get zone name
	 * 
	 * @param zoneId
	 * @return
	 */
	private String getZoneName(int zoneId)
	{
      Zone zone = Registry.getSession().findZone(zoneId);
	   if (zone == null)
	      return Long.toString(zoneId);
	   return String.format("%d (%s)", zoneId, zone.getObjectName());
	}
}
