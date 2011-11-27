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
package org.netxms.ui.eclipse.alarmviewer.actions;

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
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;


/**
 * Terminate alarm action
 *
 */
public class TerminateAlarm implements IObjectActionDelegate
{
	private class TerminateJob extends Job
	{
		private Object selection[];
		
		/**
		 * Constructor for alarm termination job.
		 * @param selection Currently selected alarms in alarm list
		 */
		TerminateJob(Object selection[])
		{
			super(Messages.TerminateAlarm_JobTitle);
			setUser(true);
			this.selection = selection;
		}

		@Override
		protected IStatus run(IProgressMonitor monitor)
		{
			IStatus status;
			
			monitor.beginTask(Messages.TerminateAlarm_TaskName, selection.length);
			try
			{
				for(int i = 0; (i < selection.length) && !monitor.isCanceled(); i++)
				{
					if (selection[i] instanceof Alarm)
						((NXCSession)ConsoleSharedData.getSession()).terminateAlarm(((Alarm)selection[i]).getId());
					monitor.worked(1);
				}
				monitor.done();
				status = Status.OK_STATUS;
			}
			catch(Exception e)
			{
				status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
				                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
				                    Messages.TerminateAlarm_ErrorMessage + e.getMessage(), e);
			}
			return status;
		}

		/* (non-Javadoc)
		 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
		 */
		@Override
		public boolean belongsTo(Object family)
		{
			return family == AlarmList.JOB_FAMILY;
		}
	}
	
	private IWorkbenchPart wbPart;
	private Object[] currentSelection;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		IWorkbenchSiteProgressService siteService =
	      (IWorkbenchSiteProgressService)wbPart.getSite().getAdapter(IWorkbenchSiteProgressService.class);
		siteService.schedule(new TerminateJob(currentSelection), 0, true);
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		wbPart = targetPart;
	}
}
