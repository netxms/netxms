package org.netxms.ui.eclipse.console;

import java.lang.reflect.InvocationTargetException;

import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.WorkbenchException;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;

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
		Shell shell = getWindowConfigurer().getWindow().getShell();
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
				MessageDialog.openError(shell, Messages.getString("NXMCWorkbenchWindowAdvisor.exception"), e.toString() + Messages.getString("NXMCWorkbenchWindowAdvisor.cause") + e.getCause().toString()); //$NON-NLS-1$ //$NON-NLS-2$
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
