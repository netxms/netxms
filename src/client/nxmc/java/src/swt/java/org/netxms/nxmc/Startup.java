/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc;

import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.security.Signature;
import java.security.cert.Certificate;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.ISafeRunnable;
import org.eclipse.core.runtime.OperationCanceledException;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.util.ISafeRunnableRunner;
import org.eclipse.jface.util.SafeRunnable;
import org.eclipse.jface.window.Window;
import org.eclipse.jface.window.Window.IExceptionHandler;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.internal.DPIUtil;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.VersionInfo;
import org.netxms.certificate.loader.KeyStoreRequestListener;
import org.netxms.certificate.manager.CertificateManager;
import org.netxms.certificate.manager.CertificateManagerProvider;
import org.netxms.certificate.request.KeyStoreEntryPasswordRequestListener;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.services.ServiceManager;
import org.netxms.nxmc.base.dialogs.PasswordExpiredDialog;
import org.netxms.nxmc.base.dialogs.PasswordRequestDialog;
import org.netxms.nxmc.base.dialogs.ReconnectDialog;
import org.netxms.nxmc.base.dialogs.SecurityWarningDialog;
import org.netxms.nxmc.base.login.LoginDialog;
import org.netxms.nxmc.base.login.LoginJob;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.base.windows.TrayIconManager;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.views.AdHocDashboardView;
import org.netxms.nxmc.modules.datacollection.SummaryTablesCache;
import org.netxms.nxmc.modules.datacollection.api.GraphTemplateCache;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.modules.logviewer.LogDescriptorRegistry;
import org.netxms.nxmc.modules.objects.MaintenanceTimePeriods;
import org.netxms.nxmc.modules.objects.ObjectIcons;
import org.netxms.nxmc.modules.objecttools.ObjectToolsCache;
import org.netxms.nxmc.modules.snmp.shared.MibCache;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Application startup code
 */
public class Startup
{
   public static Image[] windowIcons = new Image[6];

   private static I18n i18n = LocalizationHelper.getI18n(Startup.class);
   private static Logger logger = LoggerFactory.getLogger(Startup.class);
   private static ReconnectDialog reconnectDialog = null;

   /**
    * @param args
    */
   public static void main(String[] args)
   {
      final Display display = new Display();

      ServiceManager.registerClassLoader(display.getClass().getClassLoader());

      String homeDir = System.getProperty("user.home");
      File stateDir = new File(homeDir + File.separator + ".nxmc4");
      if (!stateDir.isDirectory())
      {
         stateDir.mkdir();
      }
      Registry.setStateDir(stateDir);

      logger.info("NetXMS Management Console version " + VersionInfo.version() + " starting");
      logger.info("State directory: " + stateDir.getAbsolutePath());
      logger.info("Device DPI = " + display.getDPI() + "; zoom = " + DPIUtil.getDeviceZoom());

      // Icons for application window(s)
      windowIcons[0] = ResourceManager.getImage("icons/window/256x256.png");
      windowIcons[1] = ResourceManager.getImage("icons/window/128x128.png");
      windowIcons[2] = ResourceManager.getImage("icons/window/64x64.png");
      windowIcons[3] = ResourceManager.getImage("icons/window/48x48.png");
      windowIcons[4] = ResourceManager.getImage("icons/window/32x32.png");
      windowIcons[5] = ResourceManager.getImage("icons/window/16x16.png");
      Window.setDefaultImages(windowIcons);

      PreferenceStore.open(stateDir.getAbsolutePath());
      DateFormatFactory.updateFromPreferences();
      SharedIcons.init();
      StatusDisplayInfo.init(display);
      ObjectIcons.init(display);

      Window.setExceptionHandler(new IExceptionHandler() {
         @Override
         public void handleException(Throwable t)
         {
            logger.error("Unhandled event loop exception", t);
         }
      });
      SafeRunnable.setRunner(new ISafeRunnableRunner() {
         @Override
         public void run(ISafeRunnable code)
         {
            try
            {
               code.run();
            }
            catch(Throwable t)
            {
               if (!(t instanceof OperationCanceledException))
                  logger.error("Unhandled event loop exception", t);
               code.handleException(t);
            }
         }
      });
      SafeRunnable.setIgnoreErrors(true); // Prevent display of JFace error dialog

      if (doLogin(display, args))
      {
         NXCSession session = Registry.getSession();
         DataCollectionDisplayInfo.init();
         MaintenanceTimePeriods.init(session);
         MibCache.init(session, display);
         ObjectToolsCache.init();
         ObjectToolsCache.attachSession(display, session);
         SummaryTablesCache.attachSession(display, session);
         GraphTemplateCache.attachSession(display, session);
         LogDescriptorRegistry.attachSession(display, session);
         session.addListener((n) -> {
            processSessionNotification(n);
         });
         session.enableReconnect(true);
         openWindows(session, args);
      }

      for(Image i : windowIcons)
      {
         if (i != null)
            i.dispose();
      }

      display.dispose();
      logger.info("Application exit");

      logger.debug("Running threads on shutdown:");
      for(Thread t : Thread.getAllStackTraces().keySet())
         logger.debug("   " + t.getName() + " state=" + t.getState() + " daemon=" + t.isDaemon());
   }

   /**
    * Open application window(s)
    * 
    * @param session client session
    * @param args command line arguments
    */
   private static void openWindows(NXCSession session, String[] args)
   {
      boolean kioskMode = false;
      String dashboardName = null;
      for(String s : args)
      {
         if (s.startsWith("-dashboard="))
         {
            dashboardName = s.substring(11);
         }
         else if (s.equals("-kiosk-mode"))
         {
            kioskMode = true;
         }
      }

      View fullScreenView = null;
      if (dashboardName != null)
      {
         Dashboard dashboard = (Dashboard)session.findObjectByName(dashboardName, (o) -> o instanceof Dashboard);
         if (dashboard != null)
            fullScreenView = new AdHocDashboardView(0, dashboard, null);
      }

      if (kioskMode)
      {
         PopOutViewWindow.open(fullScreenView, true, true);
      }
      else
      {
         if (PreferenceStore.getInstance().getAsBoolean("Appearance.ShowTrayIcon", true))
            TrayIconManager.showTrayIcon();

         MainWindow w = new MainWindow();
         Registry.setMainWindow(w);
         w.setBlockOnOpen(true);
         if (fullScreenView != null)
         {
            final View view = fullScreenView;
            w.setPostOpenRunnable(() -> Display.getCurrent().asyncExec(() -> PopOutViewWindow.open(view, true, false)));
         }
         w.open();
      }
   }

   /**
    * Show login dialog and perform login
    */
   private static boolean doLogin(final Display display, String[] args)
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      boolean success = false;
      boolean autoConnect = false;
      boolean ignoreProtocolVersion = false;
      String password = "";

      CertificateManager certMgr = CertificateManagerProvider.provideCertificateManager();
      certMgr.setKeyStoreRequestListener(new KeyStoreRequestListener() {
         @Override
         public String keyStorePasswordRequested()
         {
            return showPasswordRequestDialog(i18n.tr("Certificate store password"),
                  i18n.tr("The selected store is password-protected, please provide the password."));
         }

         @Override
         public String keyStoreLocationRequested()
         {
            Shell shell = Display.getCurrent().getActiveShell();

            FileDialog dialog = new FileDialog(shell);
            dialog.setText(i18n.tr("Path to the certificate store"));
            dialog.setFilterExtensions(new String[] { "*.p12; *.pfx" }); //$NON-NLS-1$
            dialog.setFilterNames(new String[] { i18n.tr("PKCS12 files (*.p12, *.pfx)") });

            return dialog.open();
         }
      });
      certMgr.setPasswordRequestListener(new KeyStoreEntryPasswordRequestListener() {
         @Override
         public String keyStoreEntryPasswordRequested()
         {
            return showPasswordRequestDialog(
                  i18n.tr("Certificate Password"),
                  i18n.tr("The selected certificate is password-protected, please provide the password."));
         }
      });

      for(String s : args)
      {
         if (s.startsWith("-server="))
         {
            settings.set("Connect.Server", s.substring(8));
         }
         else if (s.startsWith("-login="))
         {
            settings.set("Connect.Login", s.substring(7));
         }
         else if (s.startsWith("-password="))
         {
            password = s.substring(10);
            settings.set("Connect.AuthMethod", AuthenticationType.PASSWORD.getValue());
         }
         else if (s.equals("-auto")) //$NON-NLS-1$
         {
            autoConnect = true;
         }
         else if (s.equals("-ignore-protocol-version"))
         {
            ignoreProtocolVersion = true;
         }
      }

      boolean encrypt = true;
      LoginDialog loginDialog = new LoginDialog(null, certMgr);

      while(!success)
      {
         if (!autoConnect)
         {
            if (loginDialog.open() != Window.OK)
            {
               logger.info("Login cancelled by user - exiting");
               return false;
            }
            password = loginDialog.getPassword();
         }
         else
         {
            autoConnect = false; // only do auto connect first time
         }

         LoginJob job = new LoginJob(display, ignoreProtocolVersion);

         AuthenticationType authMethod = AuthenticationType.getByValue(settings.getAsInteger("Connect.AuthMethod", AuthenticationType.PASSWORD.getValue()));
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

         ProgressMonitorDialog monitorDialog = new ProgressMonitorDialog(null) {
            @Override
            protected void configureShell(Shell shell)
            {
               super.configureShell(shell);
               shell.setImages(windowIcons);
            }
         };
         try
         {
            monitorDialog.run(true, false, job);
            success = true;
         }
         catch(InvocationTargetException e)
         {
            if ((e.getCause() instanceof NXCException)
                  && ((((NXCException)e.getCause()).getErrorCode() == RCC.NO_ENCRYPTION_SUPPORT)
                        || (((NXCException)e.getCause()).getErrorCode() == RCC.NO_CIPHERS))
                  && encrypt)
            {
               boolean alwaysAllow = settings.getAsBoolean("Connect.AllowUnencrypted." + settings.getAsString("Connect.Server"), false);
               int action = getAction(settings, alwaysAllow);
               if (action != SecurityWarningDialog.NO)
               {
                  autoConnect = true;
                  encrypt = false;
                  if (action == SecurityWarningDialog.ALWAYS)
                  {
                     settings.set("Connect.AllowUnencrypted." + settings.getAsString("Connect.Server"), true);
                  }
               }
            }
            else
            {
               Throwable cause = e.getCause();
               logger.error("Login job failed", cause);
               if (!(cause instanceof NXCException) || (((NXCException)cause).getErrorCode() != RCC.OPERATION_CANCELLED))
               {
                  MessageDialog.openError(null, i18n.tr("Connection Error"), cause.getLocalizedMessage());
               }
            }
         }
         catch(Exception e)
         {
            logger.error("Unexpected exception while running login job", e);
            MessageDialog.openError(null, i18n.tr("Internal error"), e.toString());
         }
      }

      CertificateManagerProvider.dispose();

      // Suggest user to change password if it is expired
      final NXCSession session = Registry.getSession();
      if ((session.getAuthenticationMethod() == AuthenticationType.PASSWORD) && session.isPasswordExpired())
      {
         requestPasswordChange(loginDialog.getPassword(), session);
      }

      return true;
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
         logger.error("Exception in getSignature", e); //$NON-NLS-1$
         return null;
      }

      return sign;
   }

   /**
    * @param settings
    * @param alwaysAllow
    * @return
    */
   private static int getAction(PreferenceStore settings, boolean alwaysAllow)
   {
      if (alwaysAllow)
         return SecurityWarningDialog.YES;

      return SecurityWarningDialog.showSecurityWarning(null,
            String.format(i18n.tr("NetXMS server %s does not support encryption. Do you want to connect anyway?"), settings.getAsString("Connect.Server")), //$NON-NLS-1$
            i18n.tr("NetXMS server you are connecting to does not support encryption. If you countinue, information containing your credentials will be sent in clear text and could easily be read by a third party.\n\nFor assistance, contact your network administrator or the owner of the NetXMS server.\n\n"));
   }

   /**
    * @param currentPassword
    * @param session
    */
   private static void requestPasswordChange(final String currentPassword, final NXCSession session)
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
                  monitor.setTaskName(i18n.tr("Changing password..."));
                  session.setUserPassword(session.getUserId(), dlg.getPassword(), currentPassword);
                  monitor.setTaskName("");
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
            ProgressMonitorDialog monitorDialog = new ProgressMonitorDialog(null);
            monitorDialog.run(true, false, job);
            MessageDialogHelper.openInformation(null, i18n.tr("Information"), i18n.tr("Password changed successfully"));
            return;
         }
         catch(InvocationTargetException e)
         {
            MessageDialogHelper.openError(null, i18n.tr("Error"),
                  String.format(i18n.tr("Cannot change password (%s)"), e.getCause().getLocalizedMessage()));
         }
         catch(InterruptedException e)
         {
            logger.error("Unexpected exception while doing password change", e);
            MessageDialog.openError(null, i18n.tr("Internal error"), e.toString());
         }
      }
   }

   /**
    * @param title
    * @param message
    * @return
    */
   private static String showPasswordRequestDialog(String title, String message)
   {
      Shell shell = Display.getCurrent().getActiveShell();

      PasswordRequestDialog dialog = new PasswordRequestDialog(shell);
      dialog.setTitle(title);
      dialog.setMessage(message);

      if (dialog.open() == Window.OK)
         return dialog.getPassword();

      return null;
   }

   /**
    * Process session notifications
    *
    * @param n session notification
    */
   private static void processSessionNotification(SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.CONNECTION_BROKEN:
            processDisconnect(i18n.tr("communication error"));
            break;
         case SessionNotification.RECONNECT_COMPLETED:
            Display.getDefault().asyncExec(() -> {
               if (reconnectDialog != null)
               {
                  reconnectDialog.close();
                  reconnectDialog = null;
               }
            });
            break;
         case SessionNotification.RECONNECT_ATTEMPT_FAILED:
            Display.getDefault().asyncExec(() -> {
               if (reconnectDialog != null)
               {
                  logger.debug("Updating reconnect status: \"" + (String)n.getObject() + "\"");
                  reconnectDialog.updateStatusMessage((String)n.getObject());
               }
            });
            break;
         case SessionNotification.RECONNECT_STARTED:
            showReconnectDialog();
            break;
         case SessionNotification.SERVER_SHUTDOWN:
            processDisconnect(i18n.tr("server shutdown"));
            break;
         case SessionNotification.SESSION_KILLED:
            processDisconnect(i18n.tr("session terminated by administrator"));
            break;
      }
   }

   /**
    * Process session disconnect
    * 
    * @param reason reason of disconnect
    * @param display current display
    */
   private static void processDisconnect(String reason)
   {
      Display.getDefault().asyncExec(() -> {
         Shell shell = Registry.getMainWindow().getShell();
         MessageDialogHelper.openWarning(shell, i18n.tr("Disconnected"), i18n.tr("Connection with the server was lost ({0}). Application will now exit.", reason));
         shell.dispose();
      });
   }

   /**
    * Show reconnect dialog
    */
   private static void showReconnectDialog()
   {
      Display.getDefault().asyncExec(() -> {
         Shell shell = Registry.getMainWindow().getShell();
         reconnectDialog = new ReconnectDialog(shell);
         if (reconnectDialog.open() != Window.OK)
         {
            shell.dispose();
         }
      });
   }
}
