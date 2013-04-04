/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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

import java.text.NumberFormat;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.NodeLink;
import org.netxms.client.objects.ServiceCheck;
import org.netxms.client.objects.ServiceContainer;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * "General" element on object overview tab
 */
public class GeneralInfo extends TableElement
{
	/**
	 * @param parent
	 * @param object
	 */
	public GeneralInfo(Composite parent, AbstractObject object)
	{
		super(parent, object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.TableElement#fillTable()
	 */
	@Override
	protected void fillTable()
	{
		final AbstractObject object = getObject();
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		
		addPair("ID", Long.toString(object.getObjectId()));
		if (object.getGuid() != null)
			addPair("GUID", object.getGuid().toString());
		addPair("Class", object.getObjectClassName());
		addPair("Status", StatusDisplayInfo.getStatusText(object.getStatus()));
		switch(object.getObjectClass())
		{
			case AbstractObject.OBJECT_INTERFACE:
				Interface iface = (Interface)object;
				addPair("Interface Index", Integer.toString(iface.getIfIndex()));
				addPair("Interface Type", Integer.toString(iface.getIfType()));
				addPair("Description", iface.getDescription());
				addPair("Administrative State", iface.getAdminStateAsText());
				addPair("Operational State", iface.getOperStateAsText());
				addPair("MAC Address", iface.getMacAddress().toString());
				if ((iface.getFlags() & Interface.IF_PHYSICAL_PORT) != 0)
				{
					addPair("Slot/Port", Integer.toString(iface.getSlot()) + "/" + Integer.toString(iface.getPort()));
					AbstractNode node = iface.getParentNode();
					if ((node != null) && node.is8021xSupported())
					{
						addPair("802.1x PAE State", iface.getDot1xPaeStateAsText());
						addPair("802.1x Backend State", iface.getDot1xBackendStateAsText());
					}
				}
				if (!iface.getPrimaryIP().isAnyLocalAddress())
				{
					if (session.isZoningEnabled())
						addPair("Zone ID", Long.toString(iface.getZoneId()));
					addPair("IP Address", iface.getPrimaryIP().getHostAddress());
					addPair("IP Subnet Mask", iface.getSubnetMask().getHostAddress());
				}
				break;
			case AbstractObject.OBJECT_NODE:
				AbstractNode node = (AbstractNode)object;
				if (session.isZoningEnabled())
					addPair("Zone ID", Long.toString(node.getZoneId()));
				addPair("Primary Host Name", node.getPrimaryName());
				addPair("Primary IP Address", node.getPrimaryIP().getHostAddress());
				if (node.hasAgent())
					addPair("NetXMS Agent Version", node.getAgentVersion());
				addPair("System Description", node.getSystemDescription(), false);
				addPair("Platform Name", node.getPlatformName(), false);
				addPair("SNMP sysName", node.getSnmpSysName(), false);
				addPair("SNMP Object ID", node.getSnmpOID(), false);
				if ((node.getFlags() & AbstractNode.NF_IS_BRIDGE) != 0)
					addPair("Bridge Base Address", node.getBridgeBaseAddress().toString());
				addPair("Driver", node.getDriverName(), false);
				break;
			case AbstractObject.OBJECT_MOBILEDEVICE:
				MobileDevice md = (MobileDevice)object;
				if (md.getLastReportTime().getTime() == 0)
					addPair("Last Report", "never");
				else
					addPair("Last Report", RegionalSettings.getDateTimeFormat().format(md.getLastReportTime()));
				addPair("Device ID", md.getDeviceId());
				addPair("Vendor", md.getVendor());
				addPair("Model", md.getModel());
				addPair("Serial Number", md.getSerialNumber());
				addPair("Operating System", md.getOsName());
				addPair("OS Version", md.getOsVersion());
				addPair("User", md.getUserId(), false);
				if (md.getBatteryLevel() >= 0)
					addPair("Battery Level", Integer.toString(md.getBatteryLevel()) + "%");
				break;
			case AbstractObject.OBJECT_ACCESSPOINT:
				AccessPoint ap = (AccessPoint)object;
				addPair("Vendor", ap.getVendor());
				addPair("Model", ap.getModel());
				addPair("Serial Number", ap.getSerialNumber());
				addPair("MAC Address", ap.getMacAddress().toString());
				break;
			case AbstractObject.OBJECT_SUBNET:
				Subnet subnet = (Subnet)object;
				if (session.isZoningEnabled())
					addPair("Zone ID", Long.toString(subnet.getZoneId()));
				break;
			case AbstractObject.OBJECT_ZONE:
				Zone zone = (Zone)object;
				addPair("Zone ID", Long.toString(zone.getZoneId()));
				break;
			case AbstractObject.OBJECT_NODELINK:
				AbstractNode linkedNode = (AbstractNode)session.findObjectById(((NodeLink)object).getNodeId(), AbstractNode.class);
				if (linkedNode != null)
					addPair("Linked node", linkedNode.getObjectName());
			case AbstractObject.OBJECT_BUSINESSSERVICE:
			case AbstractObject.OBJECT_BUSINESSSERVICEROOT:
				ServiceContainer service = (ServiceContainer)object;
				NumberFormat nf = NumberFormat.getNumberInstance();
				nf.setMinimumFractionDigits(3);
				nf.setMaximumFractionDigits(3);
				addPair("Uptime for day", nf.format(service.getUptimeForDay()) + "%");
				addPair("Uptime for week", nf.format(service.getUptimeForWeek()) + "%");
				addPair("Uptime for month", nf.format(service.getUptimeForMonth()) + "%");
				break;
			case AbstractObject.OBJECT_SLMCHECK:
				ServiceCheck check = (ServiceCheck)object;
				addPair("Is template", check.isTemplate() ? "Yes" : "No");
				if (check.getTemplateId() != 0)
				{
					ServiceCheck tmpl = (ServiceCheck)session.findObjectById(check.getTemplateId(), ServiceCheck.class);
					if (tmpl != null)
						addPair("Template", tmpl.getObjectName());
				}
				break;
			default:
				break;
		}
		if (object.getGeolocation().getType() != GeoLocation.UNSET)
			addPair("Location", object.getGeolocation().toString());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return "General";
	}
}
