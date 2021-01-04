/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.dialogs.SendNotificationDialog;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Send Notification
 */
public class SendNotification implements IWorkbenchWindowActionDelegate
{
	private IWorkbenchWindow window;

   /**
    * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#dispose()
    */
	@Override
	public void dispose()
	{
	}

   /**
    * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#init(org.eclipse.ui.IWorkbenchWindow)
    */
	@Override
	public void init(IWorkbenchWindow window)
	{
		this.window = window;
	}

   /**
    * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
    */
	@Override
	public void run(IAction action)
	{
		if (window == null)
			return;
		
		final SendNotificationDialog dlg = new SendNotificationDialog(window.getShell());
		if (dlg.open() != Window.OK)
			return;
		
		final NXCSession session = ConsoleSharedData.getSession();
		new ConsoleJob("Send notification to" + dlg.getRecipient(), window.getActivePage().getActivePart(), Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.sendNotification(dlg.getChannelName(), dlg.getRecipient(), dlg.getSubject(), dlg.getMessage());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
                  final String message = String.format("Notification to %s has been enqueued", dlg.getRecipient());
                  MessageDialogHelper.openInformation(window.getShell(), "Send Notification", message);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return String.format("Cannot send notification to %s", dlg.getRecipient());
			}
		}.start();
	}

   /**
    * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
    */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
	}
}
