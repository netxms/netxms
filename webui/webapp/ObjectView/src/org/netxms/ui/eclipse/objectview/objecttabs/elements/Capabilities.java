/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objectview.Messages;

/**
 * "Capabilities" element for object overview page
 */
public class Capabilities extends TableElement
{
	/**
	 * @param parent
	 */
	public Capabilities(Composite parent, OverviewPageElement anchor)
	{
		super(parent, anchor);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return Messages.get().Capabilities_Title;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.TableElement#fillTable()
	 */
	@Override
	protected void fillTable()
	{
		if (!(getObject() instanceof AbstractNode))
			return;
		
		AbstractNode node = (AbstractNode)getObject();
		addFlag(Messages.get().Capabilities_FlagIsAgent, (node.getFlags() & AbstractNode.NF_IS_NATIVE_AGENT) != 0);
		addFlag(Messages.get().Capabilities_FlagIsBridge, (node.getFlags() & AbstractNode.NF_IS_BRIDGE) != 0);
		addFlag(Messages.get().Capabilities_FlagIsCDP, (node.getFlags() & AbstractNode.NF_IS_CDP) != 0);
		addFlag(Messages.get().Capabilities_FlagIsDot1x, (node.getFlags() & AbstractNode.NF_IS_8021X) != 0);
		addFlag(Messages.get().Capabilities_FlagIsLLDP, (node.getFlags() & AbstractNode.NF_IS_LLDP) != 0);
		addFlag(Messages.get().Capabilities_FlagIsNDP, (node.getFlags() & AbstractNode.NF_IS_SONMP) != 0);
		addFlag(Messages.get().Capabilities_FlagIsPrinter, (node.getFlags() & AbstractNode.NF_IS_PRINTER) != 0);
		addFlag(Messages.get().Capabilities_FlagIsRouter, (node.getFlags() & AbstractNode.NF_IS_ROUTER) != 0);
		addFlag(Messages.get().Capabilities_FlagIsSMCLP, (node.getFlags() & AbstractNode.NF_IS_SMCLP) != 0);
		addFlag(Messages.get().Capabilities_FlagIsSNMP, (node.getFlags() & AbstractNode.NF_IS_SNMP) != 0);
		addFlag(Messages.get().Capabilities_FlagIsSTP, (node.getFlags() & AbstractNode.NF_IS_STP) != 0);
		addFlag(Messages.get().Capabilities_FlagIsVRRP, (node.getFlags() & AbstractNode.NF_IS_VRRP) != 0);
		addFlag(Messages.get().Capabilities_FlagHasEntityMIB, (node.getFlags() & AbstractNode.NF_HAS_ENTITY_MIB) != 0);
		addFlag(Messages.get().Capabilities_FlagHasIfXTable, (node.getFlags() & AbstractNode.NF_HAS_IFXTABLE) != 0);
		if ((node.getFlags() & AbstractNode.NF_IS_SNMP) != 0)
		{
			addPair(Messages.get().Capabilities_SNMPPort, Integer.toString(node.getSnmpPort()));
			addPair(Messages.get().Capabilities_SNMPVersion, getSnmpVersionName(node.getSnmpVersion()));
		}
	}
	
	/**
	 * Add flag to list
	 * 
	 * @param name
	 * @param value
	 */
	private void addFlag(String name, boolean value)
	{
		addPair(name, value ? Messages.get().Capabilities_Yes : Messages.get().Capabilities_No);
	}
	
	/**
	 * Get SNMP version name from internal number
	 * 
	 * @param version
	 * @return
	 */
	private String getSnmpVersionName(int version)
	{
		switch(version)
		{
			case AbstractNode.SNMP_VERSION_1:
				return "1"; //$NON-NLS-1$
			case AbstractNode.SNMP_VERSION_2C:
				return "2c"; //$NON-NLS-1$
			case AbstractNode.SNMP_VERSION_3:
				return "3"; //$NON-NLS-1$
			default:
				return "???"; //$NON-NLS-1$
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean isApplicableForObject(AbstractObject object)
	{
		return object instanceof AbstractNode;
	}
}
