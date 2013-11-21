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
package org.netxms.ui.eclipse.policymanager.actions;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.policymanager.Activator;
import org.netxms.ui.eclipse.policymanager.Messages;
import org.netxms.ui.eclipse.policymanager.dialogs.SelectInstallTargetDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Action: deploy policy on agent
 *
 */
public class InstallPolicy implements IObjectActionDelegate
{
	private Shell shell;
	private Set<AgentPolicy> currentSelection = new HashSet<AgentPolicy>();
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		shell = targetPart.getSite().getShell();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		int initialMode = SelectInstallTargetDialog.INSTALL_ON_CURRENT;
		for(AgentPolicy p : currentSelection)
		{
			if (p.hasChildren())
			{
				initialMode = SelectInstallTargetDialog.INSTALL_ON_SELECTED;
				break;
			}
		}
		final SelectInstallTargetDialog dlg = new SelectInstallTargetDialog(shell, initialMode);
		
		if (dlg.open() == Window.OK)
		{
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			for(final AgentPolicy policy : currentSelection)
			{
				new ConsoleJob(String.format(Messages.get().InstallPolicy_JobName, policy.getObjectName()), null, Activator.PLUGIN_ID, null) {
					@Override
					protected void runInternal(IProgressMonitor monitor) throws Exception
					{
						final AbstractObject[] nodeList =
								(dlg.getInstallMode() == SelectInstallTargetDialog.INSTALL_ON_SELECTED) ?
									dlg.getSelectedObjects() : policy.getChildsAsArray();
						for(int i = 0; i < nodeList.length; i++)
							session.deployAgentPolicy(policy.getObjectId(), nodeList[i].getObjectId());
					}
	
					@Override
					protected String getErrorMessage()
					{
						return String.format(Messages.get().InstallPolicy_JobError, policy.getObjectName());
					}
				}.start();
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		currentSelection.clear();
		if (selection instanceof IStructuredSelection)
		{
			boolean enabled = true;
			for(Object o : ((IStructuredSelection)selection).toList())
			{
				if (o instanceof AgentPolicy)
				{
					currentSelection.add((AgentPolicy)o);
				}
				else
				{
					enabled = false;
					break;
				}
			}
			action.setEnabled(enabled);
		}
		else
		{
			action.setEnabled(false);
		}
	}
}
