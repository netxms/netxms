package org.netxms.ui.eclipse.console;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.WorkbenchException;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.dialogs.LoginDialog;
import org.netxms.ui.eclipse.console.dialogs.PasswordExpiredDialog;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

public class NXMCWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor
{
	private static final String PERSPECTIVE_PROPERTY_NAME = "org.netxms.ui.DefaultPerspective"; //$NON-NLS-1$
	private static final String DEFAULT_PERSPECTIVE_NAME = "org.netxms.ui.eclipse.console.DefaultPerspective"; //$NON-NLS-1$

	public NXMCWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
	{
		super(configurer);
	}

	@Override
	public ActionBarAdvisor createActionBarAdvisor(IActionBarConfigurer configurer)
	{
		return new NXMCActionBarAdvisor(configurer);
	}

	@Override
	public void preWindowOpen()
	{
		IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
		configurer.setShowCoolBar(true);
		configurer.setShowStatusLine(true);
		configurer.setShowProgressIndicator(true);
	}

	/**
	 * Overridden to maximize the window when shown.
	 */
	@Override
	public void postWindowCreate()
	{
		IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
		IWorkbenchWindow window = configurer.getWindow();
		window.getShell().setMaximized(true);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#openIntro()
	 */
	@Override
	public void openIntro()
	{
		doLogin();
	}

	/**
	 * Show login dialog and perform login
	 */
	private void doLogin()
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		final Shell shell = getWindowConfigurer().getWindow().getShell();
		boolean success = false;

		do
		{
			LoginDialog dialog = new LoginDialog(shell);
			dialog.open();
			if (!dialog.isOk())
				break;

			try
			{
				LoginJob job = new LoginJob(settings.get("Connect.Server"), settings.get("Connect.Login"), dialog //$NON-NLS-1$ //$NON-NLS-2$
						.getPassword());

				new ProgressMonitorDialog(shell).run(true, true, job);
				success = true;
			}
			catch(InvocationTargetException e)
			{
				MessageDialog.openError(shell, Messages.getString("NXMCWorkbenchWindowAdvisor.connectionError"), e.getCause().getLocalizedMessage()); //$NON-NLS-1$
			}
			catch(InterruptedException e)
			{
				MessageDialog.openError(shell, Messages.getString("NXMCWorkbenchWindowAdvisor.exception"), e.toString()); //$NON-NLS-1$
			}
			catch(Exception e)
			{
				MessageDialog.openError(shell, Messages.getString("NXMCWorkbenchWindowAdvisor.exception"), e.toString()); //$NON-NLS-1$
			}
		} while(!success);

		if (success)
		{
			Activator.getDefault().getStatusItemConnection().setText(Messages.getString("NXMCWorkbenchWindowAdvisor.connected")); //$NON-NLS-1$
			
			// Suggest user to change password if it is expired
			final NXCSession session = NXMCSharedData.getInstance().getSession();
			if (session.isPasswordExpired())
			{
				final PasswordExpiredDialog dlg = new PasswordExpiredDialog(shell);
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
										MessageDialog.openInformation(shell, "Information", "Password changed successfully");
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
			
			try
			{
				IWorkbenchWindow window = getWindowConfigurer().getWindow();
				window.getWorkbench().showPerspective(getDefaultPerspectiveName(), window);
			}
			catch(WorkbenchException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		else
		{
			shell.close();
		}
	}

	private String getDefaultPerspectiveName()
	{
		return System.getProperty(PERSPECTIVE_PROPERTY_NAME, DEFAULT_PERSPECTIVE_NAME);
	}
}
