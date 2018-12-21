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
package org.netxms.ui.eclipse.datacollection.actions;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Template;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Action: deploy policy on agent
 *
 */
public class ForceReinstallPolicy implements IObjectActionDelegate
{
	private Shell shell;
	private Set<Template> currentSelection = new HashSet<Template>();
	
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
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		for(final Template template : currentSelection)
		{
			new ConsoleJob(String.format("Force policy installation", template.getObjectName()), null, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
				   session.forcePolicyInstallation(template.getObjectId());
				   runInUIThread(new Runnable() {
                  
                  @Override
                  public void run()
                  {
                     MessageDialog.openInformation(shell, "Force policy installation", String.format("Policies for %s template were successfully installed", template.getObjectName()));
                  }
               });				   
				}

				@Override
				protected String getErrorMessage()
				{
					return String.format("Unable force install policy from %s template", template.getObjectName());
				}
			}.start();
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
				if (o instanceof Template)
				{
					currentSelection.add((Template)o);
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
