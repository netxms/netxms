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
package org.netxms.ui.eclipse.objectmanager.actions;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.dialogs.CreateNodeDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Create node object
 */
public class CreateNode implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private IWorkbenchPart part;
	private long parentId = -1;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		part = targetPart;
		window = targetPart.getSite().getWorkbenchWindow();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
	   CreateNodeDialog dlg = null;
	   do
	   {
   		dlg = new CreateNodeDialog(window.getShell(), dlg);
   		if (dlg.open() != Window.OK)
   			return;
   		
         final NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, dlg.getObjectName(), parentId);
         cd.setCreationFlags(dlg.getCreationFlags());
         cd.setPrimaryName(dlg.getHostName());
         cd.setAgentPort(dlg.getAgentPort());
         cd.setSnmpPort(dlg.getSnmpPort());
         cd.setAgentProxyId(dlg.getAgentProxy());
         cd.setSnmpProxyId(dlg.getSnmpProxy());
         cd.setZoneId(dlg.getZoneId());   		
   		
   		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   		new ConsoleJob(Messages.get().CreateNode_JobTitle, part, Activator.PLUGIN_ID, null) {
   			@Override
   			protected void runInternal(IProgressMonitor monitor) throws Exception
   			{
   				session.createObject(cd);
   			}
   
   			@Override
   			protected String getErrorMessage()
   			{
   				return String.format(Messages.get().CreateNode_JobError, cd.getName());
   			}
   		}.start();
	   } while(dlg.isShowAgain());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if ((selection instanceof IStructuredSelection) && (((IStructuredSelection)selection).size() == 1))
		{
			final Object object = ((IStructuredSelection)selection).getFirstElement();
			if ((object instanceof Container) || (object instanceof ServiceRoot) || (object instanceof Cluster))
			{
				parentId = ((AbstractObject)object).getObjectId();
			}
			else
			{
				parentId = -1;
			}
		}
		else
		{
			parentId = -1;
		}

		action.setEnabled(parentId != -1);
	}
}
