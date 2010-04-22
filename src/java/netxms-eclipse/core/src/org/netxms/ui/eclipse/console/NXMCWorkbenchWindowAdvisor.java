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
package org.netxms.ui.eclipse.console;

import java.lang.reflect.InvocationTargetException;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.dialogs.LoginDialog;
import org.netxms.ui.eclipse.console.dialogs.PasswordExpiredDialog;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

public class NXMCWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor
{
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
		doLogin();

		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		
		IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
		configurer.setShowCoolBar(ps.getBoolean("SHOW_COOLBAR")); //$NON-NLS-1$
		configurer.setShowStatusLine(true);
		configurer.setShowProgressIndicator(true);
		configurer.setShowPerspectiveBar(true);
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
		
		NXCSession session = NXMCSharedData.getInstance().getSession();
		Activator.getDefault().getStatusItemConnection().setText(session.getUserName() + "@" + session.getServerAddress() + " (" + session.getServerVersion() + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
	}

	/**
	 * Show login dialog and perform login
	 */
	private void doLogin()
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		final Shell shell = getWindowConfigurer().getWindow().getShell();
		boolean success = false;

		LoginDialog loginDialog;
		do
		{
			loginDialog = new LoginDialog(shell);
			loginDialog.open();
			if (!loginDialog.isOk())
				break;

			try
			{
				LoginJob job = new LoginJob(settings.get("Connect.Server"), settings.get("Connect.Login"), //$NON-NLS-1$ //$NON-NLS-2$
				                            loginDialog.getPassword());

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
			// Suggest user to change password if it is expired
			final NXCSession session = NXMCSharedData.getInstance().getSession();
			if (session.isPasswordExpired())
			{
				final PasswordExpiredDialog dlg = new PasswordExpiredDialog(shell);
				if (dlg.open() == Window.OK)
				{
					final String currentPassword = loginDialog.getPassword();
					IRunnableWithProgress job = new IRunnableWithProgress() {
						@Override
						public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
						{
							try
							{
								NXCSession session = NXMCSharedData.getInstance().getSession();
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
						new ProgressMonitorDialog(shell).run(true, true, job);
						MessageDialog.openInformation(shell, Messages.getString("NXMCWorkbenchWindowAdvisor.title_information"), Messages.getString("NXMCWorkbenchWindowAdvisor.passwd_changed")); //$NON-NLS-1$ //$NON-NLS-2$
					}
					catch(InvocationTargetException e)
					{
						MessageDialog.openError(shell, Messages.getString("NXMCWorkbenchWindowAdvisor.title_error"), Messages.getString("NXMCWorkbenchWindowAdvisor.cannot_change_passwd") + " " + e.getCause().getLocalizedMessage()); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
					}
					catch(InterruptedException e)
					{
						MessageDialog.openError(shell, Messages.getString("NXMCWorkbenchWindowAdvisor.exception"), e.toString()); //$NON-NLS-1$
					}
				}
			}
		}
		else
		{
			if (shell != null)
			{
				shell.close();
			}
		}
	}
}
