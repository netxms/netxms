/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * "General" element on object overview tab
 *
 */
public class GeneralInfo extends TableElement
{
	public GeneralInfo(Composite parent, GenericObject object)
	{
		super(parent, object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.TableElement#fillTable()
	 */
	@Override
	protected void fillTable()
	{
		GenericObject object = getObject();
		
		addPair("ID", Long.toString(object.getObjectId()));
		if (object.getGuid() != null)
			addPair("GUID", object.getGuid().toString());
		addPair("Class", object.getObjectClassName());
		addPair("Status", StatusDisplayInfo.getStatusText(object.getStatus()));
		switch(object.getObjectClass())
		{
			case GenericObject.OBJECT_INTERFACE:
				Interface iface = (Interface)object;
				addPair("Interface Index", Integer.toString(iface.getIfIndex()));
				addPair("Interface Type", Integer.toString(iface.getIfType()));
				addPair("Description", iface.getDescription());
				addPair("MAC Address", iface.getMacAddress().toString());
				if ((iface.getFlags() & Interface.IF_PHYSICAL_PORT) != 0)
					addPair("Slot/Port", Integer.toString(iface.getSlot()) + "/" + Integer.toString(iface.getPort()));
				if (!iface.getPrimaryIP().isAnyLocalAddress())
				{
					addPair("IP Address", iface.getPrimaryIP().getHostAddress());
					addPair("IP Subnet Mask", iface.getSubnetMask().getHostAddress());
				}
				break;
			case GenericObject.OBJECT_NODE:
				Node node = (Node)object;
				addPair("Primary IP Address", node.getPrimaryIP().getHostAddress());
				if ((node.getFlags() & Node.NF_IS_NATIVE_AGENT) != 0)
					addPair("NetXMS Agent Version", node.getAgentVersion());
				addPair("System Description", node.getSystemDescription(), false);
				addPair("Platform Name", node.getPlatformName(), false);
				addPair("SNMP sysName", node.getSnmpSysName(), false);
				addPair("SNMP Object ID", node.getSnmpOID(), false);
				addPair("Driver", node.getDriverName(), false);
				break;
			default:
				break;
		}
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
