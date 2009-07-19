/**
 * 
 */
package org.netxms.ui.eclipse.usermanager.actions;

import java.io.IOException;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.usermanager.dialogs.ChangePasswordDialog;

/**
 * @author Victor
 *
 */
public class ChangePassword implements IWorkbenchWindowActionDelegate
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
		final ChangePasswordDialog dlg = new ChangePasswordDialog(window.getShell());
		if (dlg.open() == Window.OK)
		{
			Job job = new Job("Change password for current user") {
				@Override
				protected IStatus run(IProgressMonitor monitor)
				{
					IStatus status;
					try
					{
						NXCSession session = NXMCSharedData.getInstance().getSession();
						session.setUserPassword(session.getUserId(), dlg.getPassword());
						new UIJob("Password change notification") {
							@Override
							public IStatus runInUIThread(IProgressMonitor monitor)
							{
								MessageDialog.openInformation(window.getShell(), "Information", "Password changed successfully");
								return Status.OK_STATUS;
							}
						}.schedule();
						status = Status.OK_STATUS;
					}
					catch(NXCException e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID, e.getErrorCode(),
									           "Cannot change password: " + e.getMessage(), null);
					}
					catch(IOException e)
					{
						status = new Status(Status.ERROR, Activator.PLUGIN_ID, 0,
						                    "Cannot change password: I/O error (" + e.getMessage() + ")", null);
					}
					return status;
				}
			};
			job.setUser(true);
			job.schedule();
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
