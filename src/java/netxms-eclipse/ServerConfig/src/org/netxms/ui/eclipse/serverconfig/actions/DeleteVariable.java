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
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.netxms.api.client.servermanager.ServerVariable;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.views.ServerConfigurationEditor;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author victor
 *
 */
public class DeleteVariable implements IObjectActionDelegate
{
	private IWorkbenchPart wbPart;
	private Object[] currentSelection;
	
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		wbPart = targetPart;
	}

	@Override
	public void run(IAction action)
	{
		Job job = new Job("Delete configuration variable") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					NXCSession session = NXMCSharedData.getInstance().getSession();
					for(int i = 0; i < currentSelection.length; i++)
					{
						session.deleteServerVariable(((ServerVariable)currentSelection[i]).getName());
					}
					if (wbPart instanceof ServerConfigurationEditor)
						((ServerConfigurationEditor)wbPart).refreshVariablesList();
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
					                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
					                    "Cannot delete configuration variable: " + e.getMessage(), e);
				}
				return status;
			}

			
			/* (non-Javadoc)
			 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
			 */
			@Override
			public boolean belongsTo(Object family)
			{
				return family == ServerConfigurationEditor.JOB_FAMILY;
			}
		};
		IWorkbenchSiteProgressService siteService =
	      (IWorkbenchSiteProgressService)wbPart.getSite().getAdapter(IWorkbenchSiteProgressService.class);
		siteService.schedule(job, 0, true);
	}

	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if (selection instanceof IStructuredSelection)
			currentSelection = ((IStructuredSelection)selection).toArray();
	}
}
