/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.widgets;

import java.net.InetAddress;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.dialogs.IPAddressSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * IP address selector
 *
 */
public class IPAddressSelector extends AbstractSelector
{
	private static final long serialVersionUID = 1L;

	private AbstractNode node;
	private InetAddress address;
	private NXCSession session;

	/**
	 * @param parent
	 * @param style
	 */
	public IPAddressSelector(Composite parent, int style)
	{
		super(parent, style, USE_TEXT);
		setText(Messages.IPAddressSelector_None);
		session = (NXCSession)ConsoleSharedData.getSession();
	}

	/**
	 * Set node for which list of IP addresses will be displayed.
	 * 
	 * @param nodeId node ID
	 */
	public void setNode(long nodeId)
	{
		AbstractObject object = session.findObjectById(nodeId);
		if (object instanceof AbstractNode)
		{
			node = (AbstractNode)object;
			address = node.getPrimaryIP();
			setText(address.getHostAddress());
		}
		else
		{
			node = null;
			setText(Messages.IPAddressSelector_None);
		}
	}

	/**
	 * Set node for which list of IP addresses will be displayed.
	 * 
	 * @param node node object
	 */
	public void setNode(AbstractNode node)
	{
		this.node = node;
		address = node.getPrimaryIP();
		setText(address.getHostAddress());
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		IPAddressSelectionDialog dlg = new IPAddressSelectionDialog(getShell(), node);
		if (dlg.open() == Window.OK)
		{
			address = dlg.getAddress();
			setText(address.getHostAddress());
		}
	}

	/**
	 * Get selected IP address.
	 * 
	 * @return selected IP address
	 */
	public InetAddress getAddress()
	{
		return address;
	}
}
