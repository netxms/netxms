package org.netxms.webui.core;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.window.Window;
import org.eclipse.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.netxms.api.client.Session;
import org.netxms.client.NXCSession;
import org.netxms.webui.core.dialogs.LoginDialog;
import org.netxms.webui.core.dialogs.PasswordExpiredDialog;
import org.netxms.webui.tools.RWTHelper;

/**
 * Configures the initial size and appearance of a workbench window.
 */
public class ApplicationWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor
{

	public ApplicationWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
	{
		super(configurer);
	}

	public ActionBarAdvisor createActionBarAdvisor(IActionBarConfigurer configurer)
	{
		return new ApplicationActionBarAdvisor(configurer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#preWindowOpen()
	 */
	@Override
	public void preWindowOpen()
	{
		doLogin();
		
		IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
		configurer.setInitialSize(new Point(400, 300));
		configurer.setShowCoolBar(false);
		configurer.setShowStatusLine(false);
		configurer.setTitle("NetXMS Management Console");
		configurer.setShellStyle(SWT.NO_TRIM);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowCreate()
	 */
	@Override
	public void postWindowCreate()
	{
		super.postWindowCreate();
		getWindowConfigurer().getWindow().getShell().setMaximized(true);
	}

	/**
	 * Show login dialog and perform login
	 */
	private void doLogin()
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		boolean success = false;

		LoginDialog loginDialog;
		do
		{
			loginDialog = new LoginDialog(null);
			if (loginDialog.open() != Window.OK)
				continue;
			//	System.exit(0);	// TODO: do we need to use more graceful method?

			try
			{
				LoginJob job = new LoginJob(settings.get("Connect.Server"), settings.get("Connect.Login"), //$NON-NLS-1$ //$NON-NLS-2$
				                            loginDialog.getPassword(), Display.getCurrent());

System.out.println("BEGIN");				
				//new ProgressMonitorDialog(null).run(true, false, job);
job.run(new IProgressMonitor()
{
	
	@Override
	public void worked(int work)
	{
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void subTask(String name)
	{
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void setTaskName(String name)
	{
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void setCanceled(boolean value)
	{
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public boolean isCanceled()
	{
		// TODO Auto-generated method stub
		return false;
	}
	
	@Override
	public void internalWorked(double work)
	{
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void done()
	{
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void beginTask(String name, int totalWork)
	{
		// TODO Auto-generated method stub
		
	}
});
System.out.println("DONE");				
				success = true;
			}
			catch(InvocationTargetException e)
			{
				MessageDialog.openError(null, "Connection Error", e.getCause().getLocalizedMessage());
			}
			catch(Exception e)
			{
				e.printStackTrace();
				MessageDialog.openError(null, "Exception", e.toString());
			}
		} while(!success);

		if (success)
		{
			// Suggest user to change password if it is expired
			final Session session = (Session)RWT.getSessionStore().getAttribute("netxms.session");
			if (session.isPasswordExpired())
			{
				final PasswordExpiredDialog dlg = new PasswordExpiredDialog(null);
				if (dlg.open() == Window.OK)
				{
					final String currentPassword = loginDialog.getPassword();
					final Display display = Display.getCurrent();
					IRunnableWithProgress job = new IRunnableWithProgress() {
						@Override
						public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
						{
							try
							{
								NXCSession session = (NXCSession)RWTHelper.getSessionAttribute(display, "netxms.session");
								session.setUserPassword(session.getUserId(), dlg.getPassword(), currentPassword);
							}
							catch(Exception e)
							{
								throw new InvocationTargetException(e);
							}
							finally
							{
								monitor.done();
							}
						}
					};
					try
					{
						new ProgressMonitorDialog(null).run(true, true, job);
						MessageDialog.openInformation(null, "Information", "Password changed successfully");
					}
					catch(InvocationTargetException e)
					{
						MessageDialog.openError(null, "Error", "Cannot change password: " + e.getCause().getLocalizedMessage());
					}
					catch(InterruptedException e)
					{
						MessageDialog.openError(null, "Exception", e.toString());
					}
				}
			}
		}
	}
}
