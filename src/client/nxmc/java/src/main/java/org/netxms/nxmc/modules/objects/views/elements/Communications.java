/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.WirelessDomain;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.objects.views.helpers.InterfaceListLabelProvider;
import org.xnap.commons.i18n.I18n;

/**
 * "Communications" element on object overview tab
 */
public class Communications extends TableElement
{
   private final I18n i18n = LocalizationHelper.getI18n(Communications.class);

	/**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public Communications(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
      super(parent, anchor, objectView);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (object instanceof AccessPoint) || (object instanceof Chassis) || (object instanceof Interface) || (object instanceof AbstractNode) || (object instanceof Sensor);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#fillTable()
    */
	@Override
	protected void fillTable()
	{
		final AbstractObject object = getObject();
      final NXCSession session = Registry.getSession();
      switch(object.getObjectClass())
		{
         case AbstractObject.OBJECT_ACCESSPOINT:
            AccessPoint ap = (AccessPoint)object;
            addPair(i18n.tr("MAC address"), ap.getMacAddress().toString());
            if (ap.getMacAddress() != null && !ap.getMacAddress().isNull())
            {
               String vendorMac = session.getVendorByMac(ap.getMacAddress(), null);
               if (vendorMac != null && !vendorMac.isEmpty())
                  addPair(i18n.tr("MAC Address Vendor"), vendorMac);
            }
            AbstractNode controller = session.findObjectById(ap.getControllerId(), AbstractNode.class);
            if (ap.getIpAddress().isValidAddress())
            {
               if (session.isZoningEnabled() && (controller != null))
                  addPair(i18n.tr("Zone UIN"), getZoneName(controller.getZoneId()));
               addPair(i18n.tr("IP address"), ap.getIpAddress().getHostAddress());
            }
            if (controller != null)
               addPair(i18n.tr("Controller"), controller.getObjectName());
            WirelessDomain wirelessDomain = session.findObjectById(ap.getWirelessDomainId(), WirelessDomain.class);
            if (wirelessDomain != null)
               addPair(i18n.tr("Wireless domain"), wirelessDomain.getObjectName());
            break;
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
            break;
         case AbstractObject.OBJECT_INTERFACE:
            Interface iface = (Interface)object;
            addPair(i18n.tr("MAC address"), iface.getMacAddress().toString());
            if (iface.getMacAddress() != null && !iface.getMacAddress().isNull())
            {
               String vendor = session.getVendorByMac(iface.getMacAddress(), null);
               if (vendor != null && !vendor.isEmpty())
                  addPair(i18n.tr("MAC Address Vendor"), vendor);
            }
            addPair("VLAN", InterfaceListLabelProvider.getVlanList(iface), false);
				if (iface.getIpAddressList().size() > 0)
				{
					if (session.isZoningEnabled())
                  addPair(i18n.tr("Zone UIN"), getZoneName(iface.getZoneId()));
               addPair(i18n.tr("IP address"), iface.getIpAddressList().get(0).toString());
					for(int i = 1; i < iface.getIpAddressList().size(); i++)
	               addPair("", iface.getIpAddressList().get(i).toString()); //$NON-NLS-1$
				}
            if (iface.isOSPF())
            {
               addPair(i18n.tr("OSPF area"), iface.getOSPFArea().getHostAddress());
               addPair(i18n.tr("OSPF interface type"), iface.getOSPFType().getText(), false);
            }
				break;
			case AbstractObject.OBJECT_NODE:
				AbstractNode node = (AbstractNode)object;
            if (session.isZoningEnabled())
               addPair(i18n.tr("Zone UIN"), getZoneName(node.getZoneId()));
            addPair(i18n.tr("Primary host name"), node.getPrimaryName());
            addPair(i18n.tr("Primary IP address"), node.getPrimaryIP().getHostAddress());
            if (node.isIcmpStatisticsCollected())
            {
               addPair(i18n.tr("ICMP average response time"), node.getIcmpAverageResponseTime() + " ms");
               addPair(i18n.tr("ICMP packet loss"), node.getIcmpPacketLoss() + "%");
            }
				break;
         case AbstractObject.OBJECT_SENSOR:
            Sensor sensor = (Sensor)object;
            if (sensor.getGatewayId() != 0)
            {
               AbstractNode gatewayNode = session.findObjectById(sensor.getGatewayId(), AbstractNode.class);
               if (gatewayNode != null)
               {
                  addPair(i18n.tr("Gateway"), gatewayNode.getObjectName());
               }
               addPair("Modbus Unit ID", Integer.toString(sensor.getModbusUnitId()));
            }
            addPair(i18n.tr("Device address"), sensor.getDeviceAddress(), false);
            if (sensor.getMacAddress() != null && !sensor.getMacAddress().isNull())
            {
               addPair(i18n.tr("MAC address"), sensor.getMacAddress().toString(), true);
               String vendorMac = session.getVendorByMac(sensor.getMacAddress(), null);
               if (vendorMac != null && !vendorMac.isEmpty())
                  addPair(i18n.tr("MAC Address Vendor"), vendorMac);   
            }
            break;
			default:
				break;
		}
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("Communications");
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
