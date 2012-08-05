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
 */package org.netxms.ui.eclipse.switchmanager.views.helpers;

import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;

/**
 * 802.1x port summary
 */
public class Dot1xPortSummary
{
	private Node node;
	private Interface iface;
	
	/**
	 * @param node
	 * @param iface
	 */
	public Dot1xPortSummary(Node node, Interface iface)
	{
		super();
		this.node = node;
		this.iface = iface;
	}
	
	/**
	 * Get node name
	 * 
	 * @return
	 */
	public String getNodeName()
	{
		return node.getObjectName();
	}
	
	/**
	 * Get port name
	 * 
	 * @return
	 */
	public String getPortName()
	{
		if ((iface.getFlags() & Interface.IF_PHYSICAL_PORT) != 0)
			return Integer.toString(iface.getSlot()) + "/" + Integer.toString(iface.getPort()); //$NON-NLS-1$
		return iface.getObjectName();
	}
	
	/**
	 * @return
	 */
	public String getInterfaceName()
	{
		return iface.getObjectName();
	}
	
	/**
	 * Get slot number.
	 * 
	 * @return
	 */
	public int getSlot()
	{
		return iface.getSlot();
	}
	
	/**
	 * Get port number.
	 * 
	 * @return
	 */
	public int getPort()
	{
		return iface.getPort();
	}
	
	/**
	 * Get port's PAE state
	 * 
	 * @return
	 */
	public int getPaeState()
	{
		return iface.getDot1xPaeState();
	}

	/**
	 * Get port's PAE state as text
	 * 
	 * @return
	 */
	public String getPaeStateAsText()
	{
		return iface.getDot1xPaeStateAsText();
	}

	/**
	 * Get port's authentication backend state
	 * 
	 * @return
	 */
	public int getBackendState()
	{
		return iface.getDot1xBackendState();
	}
	
	/**
	 * Get port's authentication backend state as text
	 * 
	 * @return
	 */
	public String getBackendStateAsText()
	{
		return iface.getDot1xBackendStateAsText();
	}
}
