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
package org.netxms.ui.eclipse.console.actions;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.views.ServerConsole;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Open server console
 */
public class OpenServerConsole implements IWorkbenchWindowActionDelegate
{
	private IWorkbenchWindow window;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#dispose()
	 */
	@Override
	public void dispose()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#init(org.eclipse.ui.IWorkbenchWindow)
	 */
	@Override
	public void init(IWorkbenchWindow window)
	{
		this.window = window;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		if(window != null)
		{	
			try 
			{
				if (window.getActivePage().findView(ServerConsole.ID) == null)
				{
					new ConsoleJob(Messages.getString("OpenServerConsole.JobTitle"), null, Activator.PLUGIN_ID, null) { //$NON-NLS-1$
						@Override
						protected void runInternal(IProgressMonitor monitor) throws Exception
						{
							final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
							session.openConsole();
							new UIJob(Messages.getString("OpenServerConsole.JobTitle")) { //$NON-NLS-1$
								@Override
								public IStatus runInUIThread(IProgressMonitor monitor)
								{
									try
									{
										window.getActivePage().showView(ServerConsole.ID);
									}
									catch(PartInitException e)
									{
										MessageDialog.openError(window.getShell(), Messages.getString("OpenServerConsole.Error"), Messages.getString("OpenServerConsole.ViewErrorMessage") + e.getMessage()); //$NON-NLS-1$ //$NON-NLS-2$
									}
									return Status.OK_STATUS;
								}
							}.schedule();
						}

						@Override
						protected String getErrorMessage()
						{
							return Messages.getString("OpenServerConsole.OpenErrorMessage"); //$NON-NLS-1$
						}
					}.start();
				}
				else
				{
					window.getActivePage().showView(ServerConsole.ID);
				}
			} 
			catch (PartInitException e) 
			{
				MessageDialog.openError(window.getShell(), Messages.getString("OpenServerConsole.Error"), Messages.getString("OpenServerConsole.ViewErrorMessage") + e.getMessage()); //$NON-NLS-1$ //$NON-NLS-2$
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
	}
}
