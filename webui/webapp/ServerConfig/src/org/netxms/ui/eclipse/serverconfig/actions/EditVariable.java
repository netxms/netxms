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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.netxms.api.client.servermanager.ServerManager;
import org.netxms.api.client.servermanager.ServerVariable;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.dialogs.VariableEditDialog;
import org.netxms.ui.eclipse.serverconfig.views.ServerConfigurationEditor;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Action for editing server's variable
 *
 */
public class EditVariable implements IObjectActionDelegate
{
	private IWorkbenchPart wbPart;
	private Object[] currentSelection;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		wbPart = targetPart;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		if (currentSelection.length == 1)
		{
			ServerVariable var = (ServerVariable)currentSelection[0];
			final VariableEditDialog dlg = new VariableEditDialog(wbPart.getSite().getShell(), var.getName(), var.getValue());
			if (dlg.open() == Window.OK)
			{
				final ServerManager sm = (ServerManager)ConsoleSharedData.getSession();
				new ConsoleJob("Modify configuration variable", wbPart, Activator.PLUGIN_ID, ServerConfigurationEditor.JOB_FAMILY) {
					@Override
					protected void runInternal(IProgressMonitor monitor) throws Exception
					{
						sm.setServerVariable(dlg.getVarName(), dlg.getVarValue());
						if (wbPart instanceof ServerConfigurationEditor)
							((ServerConfigurationEditor)wbPart).refreshViewer();
					}

					@Override
					protected String getErrorMessage()
					{
						return "Cannot modify configuration variable";
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
		if (selection instanceof IStructuredSelection)
			currentSelection = ((IStructuredSelection)selection).toArray();
	}
}
