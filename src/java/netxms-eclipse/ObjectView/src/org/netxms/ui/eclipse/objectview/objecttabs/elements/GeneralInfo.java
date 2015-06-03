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
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * "General" element on object overview tab
 */
public class GeneralInfo extends TableElement
{
	/**
	 * @param parent
	 */
	public GeneralInfo(Composite parent, OverviewPageElement anchor)
	{
		super(parent, anchor);
	}

	/* (non-Javadoc)
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
		addPair(Messages.get().GeneralInfo_Status, StatusDisplayInfo.getStatusText(object.getStatus()));
		switch(object.getObjectClass())
		{
			case AbstractObject.OBJECT_INTERFACE:
				Interface iface = (Interface)object;
				addPair(Messages.get().GeneralInfo_IfIndex, Integer.toString(iface.getIfIndex()));
				addPair(Messages.get().GeneralInfo_IfType, Integer.toString(iface.getIfType()));
				addPair(Messages.get().GeneralInfo_Description, iface.getDescription(), false);
            addPair("Alias", iface.getAlias(), false);
            if (iface.getMtu() > 0)
               addPair("MTU", Integer.toString(iface.getMtu()));
            addPair(Messages.get().GeneralInfo_MACAddr, iface.getMacAddress().toString());
				if ((iface.getFlags() & Interface.IF_PHYSICAL_PORT) != 0)
				{
					addPair(Messages.get().GeneralInfo_SlotPort, Integer.toString(iface.getSlot()) + "/" + Integer.toString(iface.getPort())); //$NON-NLS-1$
					AbstractNode node = iface.getParentNode();
					if ((node != null) && node.is8021xSupported())
					{
						addPair(Messages.get().GeneralInfo_8021xPAE, iface.getDot1xPaeStateAsText());
						addPair(Messages.get().GeneralInfo_8021xBackend, iface.getDot1xBackendStateAsText());
					}
				}
				if (iface.getIpAddressList().size() > 0)
				{
					if (session.isZoningEnabled())
						addPair(Messages.get().GeneralInfo_ZoneId, Long.toString(iface.getZoneId()));
					addPair(Messages.get().GeneralInfo_IPAddr, iface.getIpAddressList().get(0).toString());
					for(int i = 1; i < iface.getIpAddressList().size(); i++)
	               addPair("", iface.getIpAddressList().get(i).toString());
				}
            addPair(Messages.get().GeneralInfo_AdmState, iface.getAdminStateAsText());
            addPair(Messages.get().GeneralInfo_OperState, iface.getOperStateAsText());
				break;
			case AbstractObject.OBJECT_NODE:
				AbstractNode node = (AbstractNode)object;
				if (session.isZoningEnabled())
					addPair(Messages.get().GeneralInfo_ZoneId, Long.toString(node.getZoneId()));
				addPair(Messages.get().GeneralInfo_PrimaryHostName, node.getPrimaryName());
				addPair(Messages.get().GeneralInfo_PrimaryIP, node.getPrimaryIP().getHostAddress());
				if (node.hasAgent())
					addPair(Messages.get().GeneralInfo_AgentVersion, node.getAgentVersion());
				addPair(Messages.get().GeneralInfo_SysDescr, node.getSystemDescription(), false);
				addPair(Messages.get().GeneralInfo_PlatformName, node.getPlatformName(), false);
				addPair(Messages.get().GeneralInfo_SysName, node.getSnmpSysName(), false);
				addPair(Messages.get().GeneralInfo_SysOID, node.getSnmpOID(), false);
				if ((node.getFlags() & AbstractNode.NF_IS_BRIDGE) != 0)
					addPair(Messages.get().GeneralInfo_BridgeBaseAddress, node.getBridgeBaseAddress().toString());
				addPair(Messages.get().GeneralInfo_Driver, node.getDriverName(), false);
            if (node.getBootTime() != null)
               addPair(Messages.get().GeneralInfo_BootTime, RegionalSettings.getDateTimeFormat().format(node.getBootTime()), false);
				break;
			case AbstractObject.OBJECT_MOBILEDEVICE:
				MobileDevice md = (MobileDevice)object;
				if (md.getLastReportTime().getTime() == 0)
					addPair(Messages.get().GeneralInfo_LastReport, Messages.get().GeneralInfo_Never);
				else
					addPair(Messages.get().GeneralInfo_LastReport, RegionalSettings.getDateTimeFormat().format(md.getLastReportTime()));
				addPair(Messages.get().GeneralInfo_DeviceId, md.getDeviceId());
				addPair(Messages.get().GeneralInfo_Vendor, md.getVendor());
				addPair(Messages.get().GeneralInfo_Model, md.getModel());
				addPair(Messages.get().GeneralInfo_Serial, md.getSerialNumber());
				addPair(Messages.get().GeneralInfo_OS, md.getOsName());
				addPair(Messages.get().GeneralInfo_OSVersion, md.getOsVersion());
				addPair(Messages.get().GeneralInfo_User, md.getUserId(), false);
				if (md.getBatteryLevel() >= 0)
					addPair(Messages.get().GeneralInfo_BatteryLevel, Integer.toString(md.getBatteryLevel()) + "%"); //$NON-NLS-1$
				break;
			case AbstractObject.OBJECT_ACCESSPOINT:
				AccessPoint ap = (AccessPoint)object;
            addPair(Messages.get().GeneralInfo_State, ap.getState().toString());
				addPair(Messages.get().GeneralInfo_Vendor, ap.getVendor());
				addPair(Messages.get().GeneralInfo_Model, ap.getModel());
				addPair(Messages.get().GeneralInfo_Serial, ap.getSerialNumber());
				addPair(Messages.get().GeneralInfo_MACAddr, ap.getMacAddress().toString());
				break;
			case AbstractObject.OBJECT_SUBNET:
				Subnet subnet = (Subnet)object;
				if (session.isZoningEnabled())
					addPair(Messages.get().GeneralInfo_ZoneId, Long.toString(subnet.getZoneId()));
            addPair("IP Address", subnet.getNetworkAddress().toString());
				break;
			case AbstractObject.OBJECT_ZONE:
				Zone zone = (Zone)object;
				addPair(Messages.get().GeneralInfo_ZoneId, Long.toString(zone.getZoneId()));
				break;
			case AbstractObject.OBJECT_NODELINK:
				AbstractNode linkedNode = (AbstractNode)session.findObjectById(((NodeLink)object).getNodeId(), AbstractNode.class);
				if (linkedNode != null)
					addPair(Messages.get().GeneralInfo_LinkedNode, linkedNode.getObjectName());
			case AbstractObject.OBJECT_BUSINESSSERVICE:
			case AbstractObject.OBJECT_BUSINESSSERVICEROOT:
				ServiceContainer service = (ServiceContainer)object;
				NumberFormat nf = NumberFormat.getNumberInstance();
				nf.setMinimumFractionDigits(3);
				nf.setMaximumFractionDigits(3);
				addPair(Messages.get().GeneralInfo_UptimeDay, nf.format(service.getUptimeForDay()) + "%"); //$NON-NLS-1$
				addPair(Messages.get().GeneralInfo_UptimeWeek, nf.format(service.getUptimeForWeek()) + "%"); //$NON-NLS-1$
				addPair(Messages.get().GeneralInfo_UptimeMonth, nf.format(service.getUptimeForMonth()) + "%"); //$NON-NLS-1$
				break;
			case AbstractObject.OBJECT_SLMCHECK:
				ServiceCheck check = (ServiceCheck)object;
				addPair(Messages.get().GeneralInfo_IsTemplate, check.isTemplate() ? Messages.get().GeneralInfo_Yes : Messages.get().GeneralInfo_No);
				if (check.getTemplateId() != 0)
				{
					ServiceCheck tmpl = (ServiceCheck)session.findObjectById(check.getTemplateId(), ServiceCheck.class);
					if (tmpl != null)
						addPair(Messages.get().GeneralInfo_Template, tmpl.getObjectName());
				}
				break;
			default:
				break;
		}
		if (object.getGeolocation().getType() != GeoLocation.UNSET)
			addPair(Messages.get().GeneralInfo_Location, object.getGeolocation().toString());
		if (!object.getPostalAddress().isEmpty())
         addPair(Messages.get().GeneralInfo_PostalAddress, object.getPostalAddress().getAddressLine());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return Messages.get().GeneralInfo_Title;
	}
}
