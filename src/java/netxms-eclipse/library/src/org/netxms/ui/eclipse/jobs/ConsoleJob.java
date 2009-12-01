/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.jobs;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.netxms.client.NXCException;
import org.netxms.ui.eclipse.library.Activator;

/**
 * @author Victor
 *
 */
public abstract class ConsoleJob extends Job
{
	private IWorkbenchSiteProgressService siteService;
	private String pluginId;
	private Object jobFamily;
	
	/**
	 * Constructor for console job object
	 * 
	 * @param name Job's name
	 * @param viewPart Related view part or null
	 * @param pluginId Related plugin ID
	 * @param jobFamily Job's family or null
	 */
	public ConsoleJob(String name, ViewPart viewPart, String pluginId, Object jobFamily)
	{
		super(name);
		this.pluginId = (pluginId != null) ? pluginId : Activator.PLUGIN_ID;
		this.jobFamily = jobFamily;
		siteService = (viewPart != null) ? 
					(IWorkbenchSiteProgressService)viewPart.getSite().getAdapter(IWorkbenchSiteProgressService.class) : null;
		setUser(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.jobs.Job#run(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	protected IStatus run(IProgressMonitor monitor)
	{
		IStatus status;
		try
		{
			runInternal(monitor);
			status = Status.OK_STATUS;
		}
		catch(Exception e)
		{
			status = new Status(Status.ERROR, pluginId, 
                  (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
                  getErrorMessage() + ": " + e.getMessage(), null);
			jobFailureHandler();
		}
		return status;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.core.runtime.jobs.Job#belongsTo(java.lang.Object)
	 */
	@Override
	public boolean belongsTo(Object family)
	{
		return family == jobFamily;
	}
	
	/**
	 * Start job.
	 */
	public void start()
	{
		if (siteService != null)
			siteService.schedule(this, 0, true);
		else
			schedule();
	}

	/**
	 * Executes job. Called from within Job.run(). If job fails, this method should throw appropriate exception.
	 * 
	 * @param monitor the monitor to be used for reporting progress and responding to cancellation. The monitor is never null.
	 * @throws Exception in case of any failure.
	 */
	protected abstract void runInternal(IProgressMonitor monitor) throws Exception;
	
	/**
	 * Should return error message which will be shown in case of job failure.
	 * Result of exception's getMessage() will be appended to this message.
	 * 
	 * @return Error message
	 */
	protected abstract String getErrorMessage();
	
	/**
	 * Called from within Job.run() if job has failed. Default implementation does nothing.
	 */
	protected void jobFailureHandler()
	{		
	}
}
