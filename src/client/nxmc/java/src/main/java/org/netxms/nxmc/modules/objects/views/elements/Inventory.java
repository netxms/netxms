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
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Sensor;
import org.netxms.client.objects.Template;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * "Inventory" element on object overview tab
 */
public class Inventory extends TableElement
{
   private final I18n i18n = LocalizationHelper.getI18n(Inventory.class);

	/**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public Inventory(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
      super(parent, anchor, objectView);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (object instanceof DataCollectionTarget) || (object instanceof Template) || (object instanceof Interface);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#fillTable()
    */
	@Override
	protected void fillTable()
	{
		final AbstractObject object = getObject();
      switch(object.getObjectClass())
		{
         case AbstractObject.OBJECT_ACCESSPOINT:
            AccessPoint ap = (AccessPoint)object;
            addPair(i18n.tr("Vendor"), ap.getVendor());
            addPair(i18n.tr("Model"), ap.getModel());
            addPair(i18n.tr("Serial number"), ap.getSerialNumber());
            break;
         case AbstractObject.OBJECT_INTERFACE:
            Interface iface = (Interface)object;
            AbstractNode parentNode = iface.getParentNode();
            if (parentNode != null)
               addPair(i18n.tr("Parent node"), parentNode.getObjectName());
            Interface parentIface = iface.getParentInterface();
            if (parentIface != null)
               addPair(i18n.tr("Parent interface"), parentIface.getObjectName());
            addPair(i18n.tr("Circuits"), iface.getAllParents(AbstractObject.OBJECT_CIRCUIT).stream().map(AbstractObject::getObjectName).reduce((a, b) -> a + ", " + b).orElse(i18n.tr("")), false);
            addPair(i18n.tr("Interface index"), Integer.toString(iface.getIfIndex()));
				String typeName = iface.getIfTypeName();
            addPair(i18n.tr("Interface type"),
                  (typeName != null) ? String.format("%d (%s)", iface.getIfType(), typeName) : Integer.toString(iface.getIfType()));
            addPair(i18n.tr("Description"), iface.getDescription(), false);
            addPair(i18n.tr("Interface alias"), iface.getIfAlias(), false);
				if (iface.isPhysicalPort())
				{
               addPair(i18n.tr("Physical location"), iface.getPhysicalLocation());
				}
				break;
			case AbstractObject.OBJECT_NODE:
				AbstractNode node = (AbstractNode)object;
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
            if (node.isBridge())
               addPair(i18n.tr("Bridge base address"), node.getBridgeBaseAddress().toString());
            if (node.isOSPF())
               addPair(i18n.tr("OSPF router ID"), node.getOSPFRouterId().getHostAddress());
            if (node.hasSnmpAgent())
               addPair(i18n.tr("Driver"), node.getDriverName(), false);
				break;
			case AbstractObject.OBJECT_MOBILEDEVICE:
				MobileDevice md = (MobileDevice)object;
            addPair(i18n.tr("Device ID"), md.getDeviceId());
            addPair(i18n.tr("Vendor"), md.getVendor());
            addPair(i18n.tr("Model"), md.getModel());
            addPair(i18n.tr("Serial number"), md.getSerialNumber());
            addPair(i18n.tr("Operating System"), md.getOsName());
            addPair(i18n.tr("Operating System Version"), md.getOsVersion());
            addPair(i18n.tr("User"), md.getUserId(), false);
				break;
         case AbstractObject.OBJECT_SENSOR:
            Sensor sensor = (Sensor)object;
            addPair(i18n.tr("Device class"), sensor.getDeviceClass().getDisplayName(LocalizationHelper.getLocale()));
            addPair(i18n.tr("Vendor"), sensor.getVendor(), true);
            addPair(i18n.tr("Model"), sensor.getModel(), true);
            addPair(i18n.tr("Serial number"), sensor.getSerialNumber(), true);
            break;
         case AbstractObject.OBJECT_TEMPLATE:
            Template template = (Template)object;
            addPair(i18n.tr("Number of DCIs"), Integer.toString(template.getNumDataCollectionItems()));
            addPair(i18n.tr("Number of policies"), Integer.toString(template.getNumPolicies()));
            break;
			default:
				break;
		}
      if (object instanceof DataCollectionTarget)
      {
         addPair(i18n.tr("Number of DCIs"), Integer.toString(((DataCollectionTarget)object).getNumDataCollectionItems()));
      }
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("Inventory");
	}
}
