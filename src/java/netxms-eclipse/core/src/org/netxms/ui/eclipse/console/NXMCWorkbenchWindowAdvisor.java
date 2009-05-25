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
	private static final String PERSPECTIVE_PROPERTY_NAME = "org.netxms.ui.DefaultPerspective";
	private static final String DEFAULT_PERSPECTIVE_NAME = "org.netxms.ui.eclipse.console.DefaultPerspective";

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
		//configurer.setTitle("NetXMS Management Console");
	}

	/**
	 * Overriden to maximize the window when shwon.
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
				LoginJob job = new LoginJob(settings.get("Connect.Server"), settings.get("Connect.Login"), dialog
						.getPassword());
				new ProgressMonitorDialog(shell).run(true, true, job);
				success = true;
			}
			catch(InvocationTargetException e)
			{
				MessageDialog.openError(shell, "Exception", e.toString() + " cause: " + e.getCause().toString());
			}
			catch(InterruptedException e)
			{
				MessageDialog.openError(shell, "Exception", e.toString());
			}
			catch(Exception e)
			{
				MessageDialog.openError(shell, "Exception", e.toString());
			}
		} while(!success);

		if (success)
		{
			Activator.getDefault().getStatusItemConnection().setText("Connected");
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
