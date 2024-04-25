/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import java.security.Signature;
import java.security.cert.Certificate;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.operation.ModalContext;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.certificate.loader.KeyStoreRequestListener;
import org.netxms.certificate.manager.CertificateManager;
import org.netxms.certificate.manager.CertificateManagerProvider;
import org.netxms.certificate.request.KeyStoreEntryPasswordRequestListener;
import org.netxms.client.LicenseProblem;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.console.dialogs.LoginDialog;
import org.netxms.ui.eclipse.console.dialogs.PasswordExpiredDialog;
import org.netxms.ui.eclipse.console.dialogs.PasswordRequestDialog;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.jobs.LoginJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.Command;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Workbench window advisor
 */
public class ApplicationWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor implements KeyStoreRequestListener, KeyStoreEntryPasswordRequestListener
{
   private IDialogSettings settings;

   /**
    * @param configurer
    */
   public ApplicationWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
   {
      super(configurer);
   }

   /**
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#createActionBarAdvisor(org.eclipse.ui.application.IActionBarConfigurer)
    */
   @Override
   public ActionBarAdvisor createActionBarAdvisor(IActionBarConfigurer configurer)
   {
      return new ApplicationActionBarAdvisor(configurer);
   }

   /**
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#preWindowOpen()
    */
   @Override
   public void preWindowOpen()
   {
      doLogin(Display.getCurrent());

      RegionalSettings.updateFromPreferences();

      final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
      IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
      configurer.setShowCoolBar(ps.getBoolean("SHOW_COOLBAR")); //$NON-NLS-1$
      configurer.setShowStatusLine(true);
      configurer.setShowProgressIndicator(true);
      configurer.setShowPerspectiveBar(!ConsoleSharedData.getSession().getClientConfigurationHintAsBoolean("PerspectiveSwitcher.Enable", true));

      TweakletManager.preWindowOpen(configurer);
   }

   /**
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowCreate()
    */
   @Override
   public void postWindowCreate()
   {
      IWorkbenchWindowConfigurer configurer = getWindowConfigurer();
      configurer.setTitle(configurer.getTitle() + " - [" + settings.get("Connect.Login") + "@" + settings.get("Connect.Server") + "]"); 

      NXCSession session = ConsoleSharedData.getSession();
      final Activator activator = Activator.getDefault();
      StatusLineContributionItem statusItemConnection = (StatusLineContributionItem)activator.getStatusLine().find("ConnectionStatus");
      statusItemConnection.setImage(Activator.getImageDescriptor("icons/conn_encrypted.png")); //$NON-NLS-1$
      statusItemConnection.setText(session.getUserName()
            + "@" + session.getServerAddress() + " (" + session.getServerVersion() + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$

      ServerNameStatusLineItem statusItemServerName = (ServerNameStatusLineItem)activator.getStatusLine().find("ServerName");
      statusItemServerName.setServerInfo(session.getServerName(), session.getServerColor());

      if (activator.getPreferenceStore().getBoolean("SHOW_TRAY_ICON")) //$NON-NLS-1$
         Activator.showTrayIcon();

      session.addListener(new SessionListener() {
         @SuppressWarnings("deprecation")
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case SessionNotification.OBJECTS_OUT_OF_SYNC:
                  activator.getWorkbench().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        StatusLineContributionItem objecSyncStatus = (StatusLineContributionItem)activator.getStatusLine().find("ObjectSyncStatus");
                        objecSyncStatus.setText("Objects are out of sync");
                     }
                  });
                  break;
               case SessionNotification.OBJECTS_IN_SYNC:
                  activator.getWorkbench().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        StatusLineContributionItem objecSyncStatus = (StatusLineContributionItem)activator.getStatusLine().find("ObjectSyncStatus");
                        objecSyncStatus.setText("");
                     }
                  });
                  break;
            }
         }
      });

      TweakletManager.postWindowCreate(configurer);
   }

   /**
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#postWindowOpen()
    */
   @Override
   public void postWindowOpen()
   {
      super.postWindowOpen();

      boolean exitAfterOpen = false;
      for(String s : Platform.getCommandLineArgs())
      {
         if (s.equals("-fullscreen")) //$NON-NLS-1$
         {
				PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell().setFullScreen(true);
				Action a = ((Action)ConsoleSharedData.getProperty("FullScreenAction")); //$NON-NLS-1$
				if (a != null)
				{
					a.setChecked(true);
				}
			}
         else if (s.startsWith("-dashboard=")) //$NON-NLS-1$
         {
            showDashboard(s.substring(11), false);
         }
         else if (s.startsWith("-exit-after-open")) //$NON-NLS-1$
         {
            exitAfterOpen = true;
         }
         else if (s.startsWith("-fullscreen-dashboard=")) //$NON-NLS-1$
         {
            showDashboard(s.substring(22), true);
         }
         else if (s.startsWith("-take-map-snapshot=")) //$NON-NLS-1$
         {
            takeMapSnapshot(s.substring(19));
         }
      }
      
      if (exitAfterOpen)
      {
         new UIJob("Exit console") {
            @Override
            public IStatus runInUIThread(IProgressMonitor monitor)
            {
               PlatformUI.getWorkbench().close();
               return Status.OK_STATUS;
            }
         }.schedule(1000);
      }
      else
      {
         showMessageOfTheDay();
      }
   }

   /**
    * Take snapshot of network map
    * 
    * @param param
    */
	private void takeMapSnapshot(String param)
	{
	   final String[] parts = param.split(",");
	   if (parts.length != 2)
	      return;
	   
      NXCSession session = ConsoleSharedData.getSession();
      
      long objectId;
      try
      {
         objectId = Long.parseLong(parts[0]);
      }
      catch(NumberFormatException e)
      {
         AbstractObject object = session.findObjectByName(parts[0], (AbstractObject o) -> o instanceof NetworkMap);
         if ((object == null) || !(object instanceof NetworkMap))
         {
            MessageDialogHelper.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error, String.format("Cannot open network map %s", parts[0]));
            return;
         }
         objectId = object.getObjectId();
      }
      
      final IWorkbenchPage page = getWindowConfigurer().getWindow().getActivePage();
      try
      {
         final IViewPart view = page.showView("org.netxms.ui.eclipse.networkmaps.views.PredefinedMap", Long.toString(objectId), IWorkbenchPage.VIEW_ACTIVATE); //$NON-NLS-1$
         page.setPartState(page.getReference(view), IWorkbenchPage.STATE_MAXIMIZED);
         new UIJob("Save map to file") {
            @Override
            public IStatus runInUIThread(IProgressMonitor monitor)
            {
               ((Command)view).execute("AbstractNetworkMap/SaveToFile", parts[1]);
               page.setPartState(page.getReference(view), IWorkbenchPage.STATE_RESTORED);
               page.hideView(view);
               return Status.OK_STATUS;
            }
         }.schedule(600);
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error, String.format("Cannot open network map view %s (%s)", parts[0], e.getLocalizedMessage()));
      }
   }

   /**
    * Show dashboard
    * 
    * @param dashboardId
    */
   private void showDashboard(String dashboardId, boolean fullScreen)
   {
      NXCSession session = ConsoleSharedData.getSession();

      long objectId;
      try
      {
         objectId = Long.parseLong(dashboardId);
      }
      catch(NumberFormatException e)
      {
         AbstractObject object = session.findObjectByName(dashboardId, (AbstractObject o) -> o instanceof Dashboard);
         if ((object == null) || !(object instanceof Dashboard))
         {
            MessageDialogHelper.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error, String.format(Messages.get().ApplicationWorkbenchWindowAdvisor_CannotOpenDashboard, dashboardId));
            return;
         }
         objectId = object.getObjectId();
      }

      Dashboard dashboard = session.findObjectById(objectId, Dashboard.class);
      if (dashboard == null)
      {
         MessageDialogHelper.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error, String.format(Messages.get().ApplicationWorkbenchWindowAdvisor_CannotOpenDashboard, dashboardId));
         return;
      }

      IWorkbenchPage page = getWindowConfigurer().getWindow().getActivePage();
      try
      {
         IViewPart view = page.showView("org.netxms.ui.eclipse.dashboard.views.DashboardView", Long.toString(objectId), IWorkbenchPage.VIEW_ACTIVATE); //$NON-NLS-1$
         if (fullScreen)
         {
            ((ViewPart)view).setPartProperty("FullScreen", "true");
            new UIJob("Minimize main window") {
               @Override
               public IStatus runInUIThread(IProgressMonitor monitor)
               {
                  PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell().setMinimized(true);
                  return Status.OK_STATUS;
               }
            }.schedule(100);
         }
         else
         {
            page.setPartState(page.getReference(view), IWorkbenchPage.STATE_MAXIMIZED);
         }
      }
      catch(PartInitException e)
      {
         MessageDialogHelper.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error, String.format(Messages.get().ApplicationWorkbenchWindowAdvisor_CannotOpenDashboardType2, dashboardId, e.getLocalizedMessage()));
      }
   }

	/**
    * Show login dialog and perform login
    */
   private void doLogin(final Display display)
   {
      settings = Activator.getDefault().getDialogSettings();
      boolean success = false;
      boolean autoConnect = false;
      boolean ignoreProtocolVersion = false;
      String password = ""; //$NON-NLS-1$

      CertificateManager certMgr = CertificateManagerProvider.provideCertificateManager();
      certMgr.setKeyStoreRequestListener(this);
      certMgr.setPasswordRequestListener(this);

      for(String s : Platform.getCommandLineArgs())
      {
         if (s.startsWith("-server=")) //$NON-NLS-1$
         {
            settings.put("Connect.Server", s.substring(8)); //$NON-NLS-1$
         }
         else if (s.startsWith("-login=")) //$NON-NLS-1$
         {
            settings.put("Connect.Login", s.substring(7)); //$NON-NLS-1$
         }
         else if (s.startsWith("-password=")) //$NON-NLS-1$
         {
            password = s.substring(10);
            settings.put("Connect.AuthMethod", AuthenticationType.PASSWORD.getValue()); //$NON-NLS-1$
         }
         else if (s.equals("-auto")) //$NON-NLS-1$
         {
            autoConnect = true;
         }
         else if (s.equals("-ignore-protocol-version")) //$NON-NLS-1$
         {
            ignoreProtocolVersion = true;
         }
      }

      LoginDialog loginDialog = new LoginDialog(null, certMgr);
      while(!success)
      {
         if (!autoConnect)
         {
            if (loginDialog.open() != Window.OK)
               System.exit(0);
            password = loginDialog.getPassword();
         }
         else
         {
            autoConnect = false; // only do auto connect first time
         }

         ConsoleSharedData.setProperty("SlowLink", settings.getBoolean("Connect.SlowLink")); //$NON-NLS-1$ //$NON-NLS-2$
         LoginJob job = new LoginJob(display, 
         		settings.get("Connect.Server"), //$NON-NLS-1$ 
               settings.get("Connect.Login"), //$NON-NLS-1$
               ignoreProtocolVersion);

         AuthenticationType authMethod;
         try
         {
         	authMethod = AuthenticationType.getByValue(settings.getInt("Connect.AuthMethod")); //$NON-NLS-1$
         }
         catch(NumberFormatException e)
         {
         	authMethod = AuthenticationType.PASSWORD;
         }
         switch(authMethod)
         {
            case PASSWORD:
               job.setPassword(password);
               break;
            case CERTIFICATE:
               job.setCertificate(loginDialog.getCertificate(), getSignature(certMgr, loginDialog.getCertificate()));
               break;
            default:
               break;
         }

         try
         {
            ModalContext.run(job, true, SplashHandler.getInstance().getBundleProgressMonitor(), Display.getCurrent());
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
      }

      CertificateManagerProvider.dispose();

      // Suggest user to change password if it is expired
      final NXCSession session = ConsoleSharedData.getSession();
      if ((session.getAuthenticationMethod() == AuthenticationType.PASSWORD) && session.isPasswordExpired())
      {
         requestPasswordChange(loginDialog.getPassword(), session);
      }
   }

   /**
    * @param certMgr
    * @param cert
    * @return
    */
   private static Signature getSignature(CertificateManager certMgr, Certificate cert)
   {
      Signature sign;

      try
      {
         sign = certMgr.extractSignature(cert);
      }
      catch(Exception e)
      {
      	Activator.logError("Exception in getSignature", e); //$NON-NLS-1$
         return null;
      }

      return sign;
   }

   /**
    * @param currentPassword
    * @param session
    */
   private void requestPasswordChange(final String currentPassword, final NXCSession session)
   {
      final PasswordExpiredDialog dlg = new PasswordExpiredDialog(null, session.getGraceLogins());

      while(true)
      {
         if (dlg.open() != Window.OK)
            return;

         IRunnableWithProgress job = new IRunnableWithProgress() {
            @Override
            public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
            {
               try
               {
                  monitor.setTaskName(Messages.get().ApplicationWorkbenchWindowAdvisor_ChangingPassword);
                  session.setUserPassword(session.getUserId(), dlg.getPassword(), currentPassword);
                  monitor.setTaskName(""); //$NON-NLS-1$
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
            ModalContext.run(job, true, SplashHandler.getInstance().getBundleProgressMonitor(), Display.getCurrent());
            MessageDialog.openInformation(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Information,
                  Messages.get().ApplicationWorkbenchWindowAdvisor_PasswordChanged);
            return;
         }
         catch(InvocationTargetException e)
         {
            MessageDialog.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Error,
                  Messages.get().ApplicationWorkbenchWindowAdvisor_CannotChangePswd + " " + e.getCause().getLocalizedMessage()); //$NON-NLS-1$
         }
         catch(InterruptedException e)
         {
            MessageDialog.openError(null, Messages.get().ApplicationWorkbenchWindowAdvisor_Exception, e.toString());
         }
      }
   }

   /**
    * @see org.netxms.certificate.request.KeyStoreLocationRequestListener#keyStoreLocationRequested()
    */
   @Override
   public String keyStoreLocationRequested()
   {
      Shell shell = Display.getCurrent().getActiveShell();

      FileDialog dialog = new FileDialog(shell);
      dialog.setText(Messages.get().ApplicationWorkbenchWindowAdvisor_CertDialogTitle);
      dialog.setFilterExtensions(new String[] { "*.p12; *.pfx" }); //$NON-NLS-1$
      dialog.setFilterNames(new String[] { Messages.get().ApplicationWorkbenchWindowAdvisor_PkcsFiles });

      return dialog.open();
   }

   /**
    * @see org.netxms.certificate.request.KeyStorePasswordRequestListener#keyStorePasswordRequested()
    */
   @Override
   public String keyStorePasswordRequested()
   {
      return showPasswordRequestDialog(Messages.get().ApplicationWorkbenchWindowAdvisor_CertStorePassword,
            Messages.get().ApplicationWorkbenchWindowAdvisor_CertStorePasswordMsg);
   }

   /**
    * @see org.netxms.certificate.request.KeyStoreEntryPasswordRequestListener#keyStoreEntryPasswordRequested()
    */
   @Override
   public String keyStoreEntryPasswordRequested()
   {
      return showPasswordRequestDialog(Messages.get().ApplicationWorkbenchWindowAdvisor_CertPassword,
            Messages.get().ApplicationWorkbenchWindowAdvisor_CertPasswordMsg);
   }

   /**
    * @param title
    * @param message
    * @return
    */
   private String showPasswordRequestDialog(String title, String message)
   {
      Shell shell = Display.getCurrent().getActiveShell();

      PasswordRequestDialog dialog = new PasswordRequestDialog(shell);
      dialog.setTitle(title);
      dialog.setMessage(message);

      if (dialog.open() == Window.OK)
      {
         return dialog.getPassword();
      }
      
      return null;
   }

   /**
    * Show the message of the day message box
    */
   private void showMessageOfTheDay()
   {   
      NXCSession session = ConsoleSharedData.getSession();

      String message = session.getMessageOfTheDay();
      if (!message.isEmpty())
      {
         MessageDialog.openInformation(Display.getCurrent().getActiveShell(), "Announcement", message);
      }

      LicenseProblem[] licenseProblems = session.getLicenseProblems();
      if ((licenseProblems != null) && (licenseProblems.length > 0))
      {
         StringBuilder sb = new StringBuilder();
         for(LicenseProblem p : licenseProblems)
         {
            if (sb.length() == 0)
               sb.append("\r\n");
            sb.append(p.getDescription());
         }
         MessageDialog.openWarning(Display.getCurrent().getActiveShell(), "License Problem", sb.toString());
      }
   }
}
