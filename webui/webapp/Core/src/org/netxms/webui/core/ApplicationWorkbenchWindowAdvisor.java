/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.webui.core;

import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.InvocationTargetException;
import java.util.Properties;
import org.eclipse.core.commands.common.NotDefinedException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.bindings.BindingManager;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.window.Window;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CBanner;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.eclipse.ui.internal.keys.BindingService;
import org.eclipse.ui.keys.IBindingService;
import org.netxms.api.client.Session;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.api.LoginForm;
import org.netxms.webui.core.dialogs.PasswordExpiredDialog;

/**
 * Configures the initial size and appearance of a workbench window.
 */
@SuppressWarnings("restriction")
public class ApplicationWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor
{
	private Properties properties;
	
	/**
	 * @param configurer
	 */
	public ApplicationWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
	{
		super(configurer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#createActionBarAdvisor(org.eclipse.ui.application.IActionBarConfigurer)
	 */
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
		configurer.setShowCoolBar(true);
		configurer.setShowPerspectiveBar(true);
		configurer.setShowStatusLine(false);
		configurer.setTitle(Messages.get().ApplicationWorkbenchWindowAdvisor_AppTitle);
		configurer.setShellStyle(SWT.NO_TRIM);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowCreate()
	 */
	@Override
	public void postWindowCreate()
	{
		super.postWindowCreate();
		
		BindingService service = (BindingService)getWindowConfigurer().getWindow().getWorkbench().getService(IBindingService.class);
		BindingManager bindingManager = service.getBindingManager();
		try
		{
			bindingManager.setActiveScheme(service.getScheme("org.netxms.ui.eclipse.defaultKeyBinding")); //$NON-NLS-1$
		}
		catch(NotDefinedException e)
		{
			e.printStackTrace();
		}
		
		final Shell shell = getWindowConfigurer().getWindow().getShell(); 
		shell.setMaximized(true);
		
		for(Control ctrl : shell.getChildren())
		{
			ctrl.setData(RWT.CUSTOM_VARIANT, "gray"); //$NON-NLS-1$
			if (ctrl instanceof CBanner)
			{
				for(Control cc : ((CBanner)ctrl).getChildren())
					cc.setData(RWT.CUSTOM_VARIANT, "gray"); //$NON-NLS-1$
			}
			else if (ctrl.getClass().getName().equals("org.eclipse.swt.widgets.Composite")) //$NON-NLS-1$
			{
				for(Control cc : ((Composite)ctrl).getChildren())
					cc.setData(RWT.CUSTOM_VARIANT, "gray"); //$NON-NLS-1$
			}
		}
		
		shell.getMenuBar().setData(RWT.CUSTOM_VARIANT, "menuBar"); //$NON-NLS-1$
	}
	
	/**
	 * Connect to NetXMS server
	 * 
	 * @param server
	 * @param login
	 * @param password
	 * @return
	 */
	private boolean connectToServer(String server, String login, String password)
	{
		boolean success = false;
		try
		{
			LoginJob job = new LoginJob(server, login, password, Display.getCurrent());
			ProgressMonitorDialog pd = new ProgressMonitorDialog(null);
			pd.run(false, false, job);
			success = true;
		}
		catch(InvocationTargetException e)
		{
			MessageDialog.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_ConnectionError, e.getCause().getLocalizedMessage());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			MessageDialog.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Exception, e.toString());
		}
		return success;
	}

	/**
	 * Show login dialog and perform login
	 */
	private void doLogin()
	{
		boolean success = false;
		
		readAppProperties();
		
		String password = "";
		boolean autoLogin = (RWT.getRequest().getParameter("auto") != null); //$NON-NLS-1$
		
		String ssoTicket = RWT.getRequest().getParameter("ticket");
		if (ssoTicket != null) 
		{
			autoLogin = true;
			String server = RWT.getRequest().getParameter("server"); //$NON-NLS-1$
			if (server == null)
				server = properties.getProperty("server", "127.0.0.1"); //$NON-NLS-1$ //$NON-NLS-2$
			success = connectToServer(server, null, ssoTicket);
		}
		else if (autoLogin) 
		{
			String server = RWT.getRequest().getParameter("server"); //$NON-NLS-1$
			if (server == null)
				server = properties.getProperty("server", "127.0.0.1"); //$NON-NLS-1$ //$NON-NLS-2$
			String login = RWT.getRequest().getParameter("login"); //$NON-NLS-1$
			if (login == null)
				login = "guest";
			password = RWT.getRequest().getParameter("password"); //$NON-NLS-1$
			if (password == null)
				password = "";
			success = connectToServer(server, login, password);
		}
		
		if (!autoLogin || !success)
		{
			Window loginDialog;
			do
			{
				loginDialog = BrandingManager.getInstance().getLoginForm(null, properties);
				if (loginDialog.open() != Window.OK)
					continue;
				password = ((LoginForm)loginDialog).getPassword();
				
				success = connectToServer(properties.getProperty("server", "127.0.0.1"),  //$NON-NLS-1$ //$NON-NLS-2$ 
				                          ((LoginForm)loginDialog).getLogin(),
				                          password);
				
			} while(!success);
		}

		if (success)
		{
			// Suggest user to change password if it is expired
			final Session session = (Session)RWT.getUISession().getAttribute("netxms.session"); //$NON-NLS-1$
			if (session.isPasswordExpired())
			{
				final PasswordExpiredDialog dlg = new PasswordExpiredDialog(null);
				if (dlg.open() == Window.OK)
				{
					final String currentPassword = password;
					final Display display = Display.getCurrent();
					IRunnableWithProgress job = new IRunnableWithProgress() {
						@Override
						public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
						{
							try
							{
								NXCSession session = (NXCSession)RWT.getUISession(display).getAttribute("netxms.session"); //$NON-NLS-1$
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
						ProgressMonitorDialog pd = new ProgressMonitorDialog(null);
						pd.run(false, false, job);
						MessageDialog.openInformation(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Information, Messages.get().ApplicationWorkbenchWindowAdvisor_PasswordChanged);
					}
					catch(InvocationTargetException e)
					{
						MessageDialog.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error, Messages.get().ApplicationWorkbenchWindowAdvisor_CannotChangePswd + e.getCause().getLocalizedMessage());
					}
					catch(InterruptedException e)
					{
						MessageDialog.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Exception, e.toString());
					}
				}
			}
		}
	}
	
	/**
	 * Read application properties from nxmc.properties in the root of WAR file
	 */
	private void readAppProperties()
	{
		properties = new Properties();
		InputStream in = null;
		try
		{
			in = getClass().getResourceAsStream("nxmc.properties"); //$NON-NLS-1$
			if (in != null)
				properties.load(in);
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
		finally
		{
			if (in != null)
			{
				try
				{
					in.close();
				}
				catch(IOException e)
				{
				}
			}
		}
	}
}
