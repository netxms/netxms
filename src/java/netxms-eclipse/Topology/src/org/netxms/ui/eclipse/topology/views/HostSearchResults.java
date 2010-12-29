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
package org.netxms.ui.eclipse.topology.views;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.ConnectionPoint;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Search results for host connection searches
 *
 */
public class HostSearchResults extends ViewPart
{

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		// TODO Auto-generated method stub

	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		// TODO Auto-generated method stub

	}

	/**
	 * Show connection point information
	 * @param cp connection point information
	 */
	public static void showConnection(ConnectionPoint cp)
	{
		final Shell shell = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell();
		
		if (cp == null)
		{
			MessageDialog.openWarning(shell, "Warning", "Connection point information cannot be found");
			return;
		}
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		try
		{
			Node host = (Node)session.findObjectById(cp.getLocalNodeId());
			Node bridge = (Node)session.findObjectById(cp.getNodeId());
			Interface iface = (Interface)session.findObjectById(cp.getInterfaceId());
			
			if ((bridge != null) && (iface != null))
			{
				if (host != null)
					MessageDialog.openInformation(shell, "Connection Point", "Node " + host.getObjectName() + " is connected to network switch " + bridge.getObjectName() + " port " + iface.getObjectName());
				else
					MessageDialog.openInformation(shell, "Connection Point", "Node with MAC address ??? is connected to network switch " + bridge.getObjectName() + " port " + iface.getObjectName());
			}
			else
			{
				MessageDialog.openWarning(shell, "Warning", "Connection point information cannot be found");
			}
		}
		catch(Exception e)
		{
			MessageDialog.openWarning(shell, "Warning", "Connection point information cannot be shown: " + e.getMessage());
		}
	}
}
