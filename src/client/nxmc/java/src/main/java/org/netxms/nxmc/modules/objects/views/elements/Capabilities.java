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
package org.netxms.nxmc.modules.objects.views.elements;

import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * "Capabilities" element for object overview page
 */
public class Capabilities extends TableElement
{
   private final I18n i18n = LocalizationHelper.getI18n(Capabilities.class);

	/**
	 * @param parent
	 * @param anchor
	 * @param objectView
	 */
   public Capabilities(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
		super(parent, anchor, objectView);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("Capabilities");
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.TableElement#fillTable()
    */
	@Override
	protected void fillTable()
	{
		if (!(getObject() instanceof AbstractNode))
			return;
		
		AbstractNode node = (AbstractNode)getObject();
      addFlag("802.1x", (node.getCapabilities() & AbstractNode.NC_IS_8021X) != 0);
      addFlag(i18n.tr("Agent"), (node.getCapabilities() & AbstractNode.NC_IS_NATIVE_AGENT) != 0);
      addFlag(i18n.tr("Bridge"), (node.getCapabilities() & AbstractNode.NC_IS_BRIDGE) != 0);
      addFlag("CDP", (node.getCapabilities() & AbstractNode.NC_IS_CDP) != 0);
      addFlag("EtherNet/IP", (node.getCapabilities() & AbstractNode.NC_IS_ETHERNET_IP) != 0);
      addFlag(i18n.tr("Entity MIB"), (node.getCapabilities() & AbstractNode.NC_HAS_ENTITY_MIB) != 0);
      addFlag(i18n.tr("Emulated Entity MIB"), (node.getCapabilities() & AbstractNode.NC_EMULATED_ENTITY_MIB) != 0);
      addFlag("LLDP", (node.getCapabilities() & AbstractNode.NC_IS_LLDP) != 0);
      addFlag("Modbus TCP", (node.getCapabilities() & AbstractNode.NC_IS_MODBUS_TCP) != 0);
      addFlag("NDP", (node.getCapabilities() & AbstractNode.NC_IS_NDP) != 0);
      addFlag("OSPF", (node.getCapabilities() & AbstractNode.NC_IS_OSPF) != 0);
      addFlag(i18n.tr("Printer"), (node.getCapabilities() & AbstractNode.NC_IS_PRINTER) != 0);
      addFlag(i18n.tr("Router"), (node.getCapabilities() & AbstractNode.NC_IS_ROUTER) != 0);
      addFlag("SMCLP", (node.getCapabilities() & AbstractNode.NC_IS_SMCLP) != 0);
      addFlag("SNMP", (node.getCapabilities() & AbstractNode.NC_IS_SNMP) != 0);
      if ((node.getCapabilities() & AbstractNode.NC_IS_SNMP) != 0)
      {
         addFlag("SNMP ifXTable", (node.getCapabilities() & AbstractNode.NC_HAS_IFXTABLE) != 0);
         addPair(i18n.tr("SNMP Port"), Integer.toString(node.getSnmpPort()));
         addPair(i18n.tr("SNMP Version"), getSnmpVersionName(node.getSnmpVersion()));
      }
      addFlag("SSH", (node.getCapabilities() & AbstractNode.NC_IS_SSH) != 0);
      addFlag("STP", (node.getCapabilities() & AbstractNode.NC_IS_STP) != 0);
      addFlag(i18n.tr("User Agent"), (node.getCapabilities() & AbstractNode.NC_HAS_USER_AGENT) != 0);
      addFlag("VNC", node.hasVNC());
      addFlag("VRRP", (node.getCapabilities() & AbstractNode.NC_IS_VRRP) != 0);
      addFlag(i18n.tr("Wireless AP"), (node.getCapabilities() & AbstractNode.NC_IS_WIFI_AP) != 0);
      addFlag(i18n.tr("Wireless Controller"), (node.getCapabilities() & AbstractNode.NC_IS_WIFI_CONTROLLER) != 0);
	}

	/**
	 * Add flag to list
	 * 
	 * @param name
	 * @param value
	 */
	private void addFlag(String name, boolean value)
	{
      addPair(name, value ? i18n.tr("Yes") : i18n.tr("No"));
	}
	
	/**
	 * Get SNMP version name from internal number
	 * 
	 * @param version
	 * @return
	 */
   private String getSnmpVersionName(SnmpVersion version)
	{
		switch(version)
		{
         case V1:
            return "1";
         case V2C:
            return "2c";
         case V3:
            return "3";
			default:
            return "???";
		}
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
	@Override
	public boolean isApplicableForObject(AbstractObject object)
	{
		return object instanceof AbstractNode;
	}
}
