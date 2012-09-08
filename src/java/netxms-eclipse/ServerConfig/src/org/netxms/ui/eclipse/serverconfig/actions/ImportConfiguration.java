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
package org.netxms.ui.eclipse.serverconfig.actions;

import java.io.FileInputStream;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.dialogs.ConfigurationImportDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Import configuration from XML
 */
public class ImportConfiguration implements IWorkbenchWindowActionDelegate
{
	private Shell shell;
	
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
		shell = window.getShell();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		final ConfigurationImportDialog dlg = new ConfigurationImportDialog(shell);
		if (dlg.open() == Window.OK)
		{
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			ConsoleJob job = new ConsoleJob("Import Configuration", null, Activator.PLUGIN_ID, null) {
				@Override
				protected String getErrorMessage()
				{
					return "Cannot import configuration";
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					FileInputStream file = new FileInputStream(dlg.getFileName());
					byte[] data = new byte[(int)file.getChannel().size()];
					file.read(data);
					file.close();
					
					String content = new String(data, "UTF-8");
					session.importConfiguration(content, dlg.getFlags());
					
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							MessageDialog.openInformation(shell, "Information", "Configuration was successfully imported from file " + dlg.getFileName());
						}
					});
				}
			};
			job.start();
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
