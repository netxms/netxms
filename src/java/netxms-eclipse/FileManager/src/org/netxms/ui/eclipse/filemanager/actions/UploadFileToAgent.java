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
 */package org.netxms.ui.eclipse.filemanager.actions;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IActionDelegate;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.ui.eclipse.filemanager.Activator;
import org.netxms.ui.eclipse.filemanager.Messages;
import org.netxms.ui.eclipse.filemanager.dialogs.StartServerToAgentFileUploadDialog;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Action: upload file from server to agent(s)
 *
 */
public class UploadFileToAgent implements IObjectActionDelegate
{
	private Shell shell;
	private ViewPart viewPart;
	private Set<Long> nodes = new HashSet<Long>();

	/**
	 * @see IObjectActionDelegate#setActivePart(IAction, IWorkbenchPart)
	 */
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		shell = targetPart.getSite().getShell();
		viewPart = (targetPart instanceof ViewPart) ? (ViewPart)targetPart : null;
	}

	/**
	 * @see IActionDelegate#run(IAction)
	 */
	public void run(IAction action)
	{
		final StartServerToAgentFileUploadDialog dlg = new StartServerToAgentFileUploadDialog(shell);
		if (dlg.open() == Window.OK)
		{
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			final Long[] nodeIdList = nodes.toArray(new Long[nodes.size()]);
			new ConsoleJob(Messages.get().UploadFileToAgent_JobTitle, viewPart, Activator.PLUGIN_ID, null) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().UploadFileToAgent_JobError;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
				   String remoteFileName = dlg.getRemoteFileName();
				   if(!remoteFileName.isEmpty())
				   {
				      if(remoteFileName.endsWith("/") || remoteFileName.endsWith("\\"))
				      {
				         remoteFileName += dlg.getServerFile().getName();
				      }
				   }
				   else 
				   {
				      remoteFileName = null;
				   }
					for(int i = 0; i < nodeIdList.length; i++)
						session.uploadFileToAgent(nodeIdList[i], dlg.getServerFile().getName(), remoteFileName, dlg.isCreateJobOnHold());
				}
			}.start();
		}
	}

	/**
	 * @see IActionDelegate#selectionChanged(IAction, ISelection)
	 */
	@SuppressWarnings("rawtypes")
	public void selectionChanged(IAction action, ISelection selection)
	{
		nodes.clear();
		
		if (!(selection instanceof IStructuredSelection))
		{
			action.setEnabled(false);
			return;
		}
		
		Iterator it = ((IStructuredSelection)selection).iterator();
		while(it.hasNext())
		{
			Object object = it.next();
			if (object instanceof Node)
			{
				nodes.add(((Node)object).getObjectId());
			}
			else if ((object instanceof Container) || (object instanceof ServiceRoot) || 
			         (object instanceof Subnet)  || (object instanceof EntireNetwork))
			{
				Set<AbstractObject> set = ((AbstractObject)object).getAllChilds(AbstractObject.OBJECT_NODE);
				for(AbstractObject o : set)
					nodes.add(o.getObjectId());
			}
		}
		
		action.setEnabled(nodes.size() > 0);
	}
}
