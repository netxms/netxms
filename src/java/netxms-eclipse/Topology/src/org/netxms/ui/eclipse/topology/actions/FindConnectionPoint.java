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
package org.netxms.ui.eclipse.topology.actions;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.ConnectionPoint;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.topology.Activator;

/**
 * Find connection point for node or interface
 *
 */
public class FindConnectionPoint implements IObjectActionDelegate
{
	private IWorkbenchPart wbPart;
	private long objectId;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Find connection point for object " + objectId, wbPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final ConnectionPoint cp = session.findConnectionPoint(objectId);
				new UIJob("Show connection point") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						showConnection(cp);
						return Status.OK_STATUS;
					}
				}.schedule();
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot get conection point information";
			}
		}.start();
	}
	
	/**
	 * Show connection point information
	 * @param cp connection point information
	 */
	private void showConnection(ConnectionPoint cp)
	{
		if (cp == null)
		{
			MessageDialog.openWarning(wbPart.getSite().getShell(), "Warning", "Connection point information cannot be found");
			return;
		}
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		try
		{
			GenericObject host = session.findObjectById(objectId);
			if (host instanceof Interface)
				host = host.getParentsAsArray()[0];
			Node bridge = (Node)session.findObjectById(cp.getNodeId());
			Interface iface = (Interface)session.findObjectById(cp.getInterfaceId());
			
			if ((bridge != null) && (iface != null))
			{
				MessageDialog.openInformation(wbPart.getSite().getShell(), "Connection Point", "Node " + host.getObjectName() + " is connected to network switch " + bridge.getObjectName() + " port " + iface.getObjectName());
			}
			else
			{
				MessageDialog.openWarning(wbPart.getSite().getShell(), "Warning", "Connection point information cannot be found");
			}
		}
		catch(Exception e)
		{
			MessageDialog.openWarning(wbPart.getSite().getShell(), "Warning", "Connection point information cannot be shown: " + e.getMessage());
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if (selection instanceof IStructuredSelection)
		{
			Object obj = ((IStructuredSelection)selection).getFirstElement();
			if ((obj instanceof Node) || (obj instanceof Interface))
			{
				action.setEnabled(true);
				objectId = ((GenericObject)obj).getObjectId();
			}
			else
			{
				action.setEnabled(false);
				objectId = 0;
			}
		}
		else
		{
			action.setEnabled(false);
			objectId = 0;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		wbPart = targetPart;
	}
}
