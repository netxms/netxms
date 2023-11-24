/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.window.Window;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.ExitConfirmation;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.rap.rwt.internal.service.UISessionImpl;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IPerspectiveDescriptor;
import org.eclipse.ui.IWindowListener;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.WorkbenchException;
import org.eclipse.ui.application.IWorkbenchConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchAdvisor;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.eclipse.ui.internal.StartupThreading;
import org.eclipse.ui.internal.StartupThreading.StartupRunnable;
import org.eclipse.ui.internal.progress.ProgressManager;
import org.eclipse.ui.model.ContributionComparator;
import org.eclipse.ui.model.IContributionService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;
import org.netxms.ui.eclipse.console.api.LoginForm;
import org.netxms.ui.eclipse.console.dialogs.PasswordExpiredDialog;
import org.netxms.ui.eclipse.console.dialogs.SelectPerspectiveDialog;
import org.netxms.ui.eclipse.jobs.LoginJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.PerspectiveSwitcher;

/**
 * Workbench advisor for NetXMS console application
 */
@SuppressWarnings("restriction")
public class ApplicationWorkbenchAdvisor extends WorkbenchAdvisor
{
   private static final String PERSPECTIVE_ID = "org.netxms.ui.eclipse.console.ManagementPerspective"; //$NON-NLS-1$
   
   /**
    * @see org.eclipse.ui.application.WorkbenchAdvisor#createWorkbenchWindowAdvisor(org.eclipse.ui.application.IWorkbenchWindowConfigurer)
    */
	public WorkbenchWindowAdvisor createWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
	{
		return new ApplicationWorkbenchWindowAdvisor(configurer);
	}

   /**
    * @see org.eclipse.ui.application.WorkbenchAdvisor#getInitialWindowPerspectiveId()
    */
	public String getInitialWindowPerspectiveId()
	{
		String p = BrandingManager.getInstance().getDefaultPerspective();
		return (p != null) ? p : PERSPECTIVE_ID;
	}

   /**
    * @see org.eclipse.ui.application.WorkbenchAdvisor#initialize(org.eclipse.ui.application.IWorkbenchConfigurer)
    */
	@Override
	public void initialize(IWorkbenchConfigurer configurer)
	{
		super.initialize(configurer);
		configurer.setSaveAndRestore(true);

      PlatformUI.getWorkbench().addWindowListener(new IWindowListener() {
         private Set<IWorkbenchWindow> windows = new HashSet<>();

         @Override
         public void windowOpened(IWorkbenchWindow window)
         {
            NXCSession session = ConsoleSharedData.getSession();
            if (session.getClientConfigurationHintAsBoolean("PerspectiveSwitcher.Enable", true) && session.getClientConfigurationHintAsBoolean("PerspectiveSwitcher.ShowOnStartup", false))
            {
               new UIJob("Select perspective") {
                  @Override
                  public IStatus runInUIThread(IProgressMonitor monitor)
                  {
                     IPerspectiveDescriptor currPerspective = window.getActivePage().getPerspective();
                     IPerspectiveDescriptor p = PlatformUI.getWorkbench().getPerspectiveRegistry().findPerspectiveWithId("org.netxms.ui.eclipse.console.SwitcherPerspective");
                     if (p != null)
                        window.getActivePage().setPerspective(p);
                     SelectPerspectiveDialog dlg = new SelectPerspectiveDialog(null);
                     if (dlg.open() == Window.OK)
                     {
                        window.getActivePage().setPerspective(dlg.getSelectedPerspective());
                     }
                     else
                     {
                        window.getActivePage().setPerspective(currPerspective);
                     }
                     return Status.OK_STATUS;
                  }
               }.schedule();
            }
         }

         @Override
         public void windowDeactivated(IWorkbenchWindow window)
         {
         }

         @Override
         public void windowClosed(IWorkbenchWindow window)
         {
            windows.remove(window);
         }

         @Override
         public void windowActivated(IWorkbenchWindow window)
         {
            NXCSession session = ConsoleSharedData.getSession();
            if (!windows.contains(window) && session.getClientConfigurationHintAsBoolean("PerspectiveSwitcher.Enable", true))
            {
               new PerspectiveSwitcher(window);
               windows.add(window);
            }
         }
      });
	}

	/**
	 * @see org.eclipse.ui.application.WorkbenchAdvisor#openWindows()
	 */
	@Override
	public boolean openWindows()
	{
		doLogin();

		// Workaround for deadlock within RAP on startup
		try
      {
         StartupThreading.runWithWorkbenchExceptions(new StartupRunnable() {
            @Override
            public void runWithException() throws Throwable
            {
               ProgressManager.getInstance();
            }
         });
      }
      catch(WorkbenchException e)
      {
         Activator.logError("Initialization error", e);
      }

		return super.openWindows();
	}

	/**
	 * @see org.eclipse.ui.application.WorkbenchAdvisor#getComparatorFor(java.lang.String)
	 */
	@Override
	public ContributionComparator getComparatorFor(String contributionType)
	{
		if (contributionType.equals(IContributionService.TYPE_PROPERTY))
			return new ExtendedContributionComparator();
		return super.getComparatorFor(contributionType);
	}

	/**
	 * Connect to NetXMS server
	 * 
	 * @param server
	 * @param login
	 * @param password
	 * @return
	 */
	private boolean connectToServer(String server, String login, String password, boolean encryptSession, boolean ignoreProtocolVersion)
	{
		boolean success = false;
		try
		{
			LoginJob job = new LoginJob(Display.getCurrent(), server, login, encryptSession, ignoreProtocolVersion);
			job.setPassword(password);
			ProgressMonitorWindow progressMonitor = new ProgressMonitorWindow(Display.getCurrent().getActiveShell());
			progressMonitor.run(job);
			success = true;
		}
		catch(InvocationTargetException e)
		{
		   Throwable cause = e.getCause();
		   Activator.logError("Login job failed", cause);
         if (!(cause instanceof NXCException) || (((NXCException)cause).getErrorCode() != RCC.OPERATION_CANCELLED))
         {
            MessageDialog.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_ConnectionError, cause.getLocalizedMessage());
         }
		}
		catch(Exception e)
		{
         Activator.logError("Error running login job", e);
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

		final AppPropertiesLoader properties = new AppPropertiesLoader();

		String password = "";
		boolean autoLogin = (Application.getParameter("auto") != null); //$NON-NLS-1$
		
		String ssoTicket = Application.getParameter("ticket");
		if (ssoTicket != null) 
		{
			autoLogin = true;
			String server = Application.getParameter("server"); //$NON-NLS-1$
			if (server == null)
				server = properties.getProperty("server", "127.0.0.1"); //$NON-NLS-1$ //$NON-NLS-2$
			success = connectToServer(server, null, ssoTicket, properties.getPropertyAsBoolean("useEncryption", true), properties.getPropertyAsBoolean("ignoreProtocolVersion", false));
		}
		else if (autoLogin) 
		{
			String server = Application.getParameter("server"); //$NON-NLS-1$
			if (server == null)
				server = properties.getProperty("server", "127.0.0.1"); //$NON-NLS-1$ //$NON-NLS-2$
			String login = Application.getParameter("login"); //$NON-NLS-1$
			if (login == null)
				login = "guest";
			password = Application.getParameter("password"); //$NON-NLS-1$
			if (password == null)
				password = "";
			success = connectToServer(server, login, password, properties.getPropertyAsBoolean("useEncryption", true), properties.getPropertyAsBoolean("ignoreProtocolVersion", false));
		}

		if (!autoLogin || !success)
		{
			Window loginDialog;
			do
			{
				loginDialog = BrandingManager.getInstance().getLoginForm(Display.getCurrent().getActiveShell(), properties);
				if (loginDialog.open() != Window.OK)
					continue;
				password = ((LoginForm)loginDialog).getPassword();
				
				success = connectToServer(properties.getProperty("server", "127.0.0.1"),  //$NON-NLS-1$ //$NON-NLS-2$ 
				                          ((LoginForm)loginDialog).getLogin(),
				                          password, properties.getPropertyAsBoolean("useEncryption", true),
				                          properties.getPropertyAsBoolean("ignoreProtocolVersion", false));
			} while(!success);
		}

		if (!success)
		   return;

		final NXCSession session = (NXCSession)RWT.getUISession().getAttribute(ConsoleSharedData.ATTRIBUTE_SESSION);
      final Display display = Display.getCurrent();
      session.addListener(new SessionListener() {
         private final Object MONITOR = new Object();

         @Override
         public void notificationHandler(final SessionNotification n)
         {
			if ((n.getCode() == SessionNotification.CONNECTION_BROKEN) ||
			    (n.getCode() == SessionNotification.SERVER_SHUTDOWN) ||
			    (n.getCode() == SessionNotification.SESSION_KILLED))
            {
               display.asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     RWT.getUISession().setAttribute("NoPageReload", Boolean.TRUE);
                     PlatformUI.getWorkbench().close();

                     String productName = BrandingManager.getInstance().getProductName();
                     String msg =
                           (n.getCode() == SessionNotification.CONNECTION_BROKEN) ? 
                         		  String.format(Messages.get().ApplicationWorkbenchAdvisor_ConnectionLostMessage, productName)
                                   : ((n.getCode() == SessionNotification.SESSION_KILLED) ?
                                     "Communication session was terminated by system administrator"
                                 	: String.format(Messages.get().ApplicationWorkbenchAdvisor_ServerShutdownMessage, productName));

                     JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
                     if (executor != null)
                        executor.execute("document.body.innerHTML='" +
                              "<div style=\"position: absolute; top: 0; left: 0; width: 100%; height: 100%; background-image: none; background-color: white;\">" + 
                              "<p style=\"margin-top: 30px; margin-left: 30px; color: #800000; font-family: \"Segoe UI\", Verdana, Arial, sans-serif; font-weight: bold; font-size: 2em;\">" + 
                              msg + 
                              "</p><br/>" + 
                              "<button style=\"margin-left: 30px\" onclick=\"location.reload(true)\">RELOAD</button>" + 
                              "</div>';");
                     
                     synchronized(MONITOR)
                     {
                        MONITOR.notifyAll();
                     }
                  }
               });

               synchronized(MONITOR)
               {
                  try
                  {
                     MONITOR.wait(5000);
                  }
                  catch(InterruptedException e)
                  {
                  }
               }
               ((UISessionImpl)RWT.getUISession(display)).shutdown();
            }
         }
      });

		try
		{
			RWT.getSettingStore().loadById(session.getUserName() + "@" + session.getServerId());
		}
		catch(IOException e)
		{
		}

		// Suggest user to change password if it is expired
		if (session.isPasswordExpired())
		{
			final PasswordExpiredDialog dlg = new PasswordExpiredDialog(null, session.getGraceLogins());
			while(true)
			{
				if (dlg.open() != Window.OK)
				   return;

				final String currentPassword = password;
				IRunnableWithProgress job = new IRunnableWithProgress() {
					@Override
					public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
					{
						try
						{
							NXCSession session = (NXCSession)RWT.getUISession(display).getAttribute(ConsoleSharedData.ATTRIBUTE_SESSION);
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
					return;
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

	   ExitConfirmation exitConfirmation = RWT.getClient().getService(ExitConfirmation.class);
	   exitConfirmation.setMessage("This will terminate your current session. Are you sure?");
	}
}
