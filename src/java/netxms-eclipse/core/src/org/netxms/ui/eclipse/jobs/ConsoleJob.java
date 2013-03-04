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
package org.netxms.ui.eclipse.jobs;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.progress.IProgressService;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.netxms.api.client.NetXMSClientException;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Tailored Job class for NetXMS console. Callers must call start() instead of schedule() for correct execution.
 *
 */
public abstract class ConsoleJob extends Job
{
	private IWorkbenchSiteProgressService siteService;
	private String pluginId;
	private Object jobFamily;
	private boolean passException = false;
	private boolean printException = true;
	
	/**
	 * Constructor for console job object
	 * 
	 * @param name Job's name
	 * @param viewPart Related view part or null
	 * @param pluginId Related plugin ID
	 * @param jobFamily Job's family or null
	 */
	public ConsoleJob(String name, IWorkbenchPart wbPart, String pluginId, Object jobFamily)
	{
		super(name);
		this.pluginId = (pluginId != null) ? pluginId : Activator.PLUGIN_ID;
		this.jobFamily = jobFamily;
		try
		{
			IWorkbenchPart part = (wbPart != null) ? wbPart : PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage().getActivePart();
			siteService = (part != null) ? 
						(IWorkbenchSiteProgressService)wbPart.getSite().getService(IWorkbenchSiteProgressService.class) : null;
		}
		catch(NullPointerException e)
		{
			siteService = null;
		}
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
			if (printException)
				e.printStackTrace();
			jobFailureHandler();
			status = createFailureStatus(e);
		}
		finally
		{
			jobFinalize();
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
	 * Called from within Job.run() if job has failed to create failure status. Subclasses may override
	 * and return IStatus.OK_STATUS to avoid standard job failure message to pop up.
	 */
	protected IStatus createFailureStatus(Exception e)
	{		
		return new Status(Status.ERROR, pluginId, 
            (e instanceof NetXMSClientException) ? ((NetXMSClientException)e).getErrorCode() : 0,
            getErrorMessage() + ": " + e.getMessage(), passException ? e : null); //$NON-NLS-1$
	}
	
	/**
	 * Called from within Job.run() if job has failed. Default implementation does nothing.
	 */
	protected void jobFailureHandler()
	{		
	}
	
	/**
	 * Called from within Job.run() if job completes, either successfully or not. Default implementation does nothing.
	 */
	protected void jobFinalize()
	{
	}

	/**
	 * Run job in foreground using IProgressService.busyCursorWhile
	 * 
	 * @return true if job was successful
	 */
	public boolean runInForeground()
	{
		passException = true;
		IProgressService service = PlatformUI.getWorkbench().getProgressService();
		boolean success = true;
		try
		{
			service.run(true, false, new IRunnableWithProgress() {
				@Override
				public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
				{
					IStatus status = ConsoleJob.this.run(monitor);
					if (!status.isOK())
					{
						throw new InvocationTargetException(status.getException());
					}
				}
			});
		}
		catch(InvocationTargetException e)
		{
			Throwable cause = e.getCause();
			if (cause == null)
				cause = e;
			MessageDialogHelper.openError(null, Messages.ConsoleJob_ErrorDialogTitle, getErrorMessage() + ": " + cause.getLocalizedMessage()); //$NON-NLS-1$
		}
		catch(InterruptedException e)
		{
		}
		return success;
	}
	
	/**
	 * Run code in UI thread
	 * 
	 * @param runnable
	 */
	protected void runInUIThread(final Runnable runnable)
	{
		PlatformUI.getWorkbench().getDisplay().asyncExec(runnable);
	}
	
	/**
	 * @return
	 */
	protected Display getDisplay()
	{
		return PlatformUI.getWorkbench().getDisplay();
	}

	/**
	 * @return the printException
	 */
	public boolean isPrintException()
	{
		return printException;
	}

	/**
	 * @param printException the printException to set
	 */
	public void setPrintException(boolean printException)
	{
		this.printException = printException;
	}
}
