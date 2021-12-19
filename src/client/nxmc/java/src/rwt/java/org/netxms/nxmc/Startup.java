/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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

import static org.eclipse.rap.rwt.RWT.getClient;
import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.security.Signature;
import java.security.cert.Certificate;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.window.Window;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.application.EntryPoint;
import org.eclipse.rap.rwt.client.service.StartupParameters;
import org.eclipse.rap.rwt.internal.application.ApplicationContextImpl;
import org.eclipse.rap.rwt.internal.service.StartupPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.VersionInfo;
import org.netxms.certificate.loader.KeyStoreRequestListener;
import org.netxms.certificate.manager.CertificateManager;
import org.netxms.certificate.manager.CertificateManagerProvider;
import org.netxms.certificate.request.KeyStoreEntryPasswordRequestListener;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.nxmc.base.dialogs.PasswordExpiredDialog;
import org.netxms.nxmc.base.dialogs.PasswordRequestDialog;
import org.netxms.nxmc.base.login.LoginDialog;
import org.netxms.nxmc.base.login.LoginJob;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.modules.objects.ObjectToolsCache;
import org.netxms.nxmc.modules.snmp.shared.MibCache;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Application startup code
 */
public class Startup implements EntryPoint, StartupParameters
{
   private static Logger logger = LoggerFactory.getLogger(Startup.class);

   private I18n i18n = LocalizationHelper.getI18n(Startup.class);
   private Display display;
   private Shell shell;

   /**
    * @see org.eclipse.rap.rwt.application.EntryPoint#createUI()
    */
   @Override
   public int createUI()
   {
      display = new Display();

      File tempDir = (File)RWT.getUISession().getHttpSession().getServletContext().getAttribute("javax.servlet.context.tempdir");
      File stateDir = new File(tempDir + File.separator + "state");
      if (!stateDir.isDirectory())
      {
         stateDir.mkdir();
      }
      Registry.getInstance().setStateDir(stateDir);

      logger.info("NetXMS Management Console version " + VersionInfo.version() + " starting");
      logger.info("State directory: " + stateDir.getAbsolutePath());

      StringBuilder sb = new StringBuilder();
      for(String themeId : ((ApplicationContextImpl)RWT.getApplicationContext()).getThemeManager().getRegisteredThemeIds())
      {
         if (sb.length() > 0)
            sb.append(", ");
         sb.append(themeId);
      }
      logger.info("Registered themes: " + sb.toString());

      PreferenceStore.open(stateDir.getAbsolutePath());
      SharedIcons.init();
      BrandingManager.create();
      StatusDisplayInfo.init(display);

      Shell shell = new Shell(display, SWT.NO_TRIM);
      shell.setMaximized(true);
      shell.open();

      if (!doLogin(new String[0])) // FIXME: parse arguments
      {
         logger.info("Application instance exit");
         display.dispose();
         return 0;
      }

      DataCollectionDisplayInfo.init();
      MibCache.init(Registry.getSession(), display);
      ObjectToolsCache.init();
      ObjectToolsCache.attachSession(Registry.getSession());

      MainWindow w = new MainWindow(shell);
      Registry.getInstance().setMainWindow(w);
      w.setBlockOnOpen(true);
      w.open();

      logger.info("Application instance exit");
      display.dispose();

      return 0;
   }

   /**
    * Show login dialog and perform login
    */
   private boolean doLogin(String[] args)
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      boolean success = false;
      boolean autoConnect = false;
      boolean ignoreProtocolVersion = false;
      String password = ""; //$NON-NLS-1$

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
            FileDialog dialog = new FileDialog(shell);
            dialog.setText(i18n.tr("Path to the certificate store"));
            dialog.setFilterExtensions(new String[] { "*.p12; *.pfx" }); //$NON-NLS-1$

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
         if (s.startsWith("-server=")) //$NON-NLS-1$
         {
            settings.set("Connect.Server", s.substring(8)); //$NON-NLS-1$
         }
         else if (s.startsWith("-login=")) //$NON-NLS-1$
         {
            settings.set("Connect.Login", s.substring(7)); //$NON-NLS-1$
         }
         else if (s.startsWith("-password=")) //$NON-NLS-1$
         {
            password = s.substring(10);
            settings.set("Connect.AuthMethod", AuthenticationType.PASSWORD.getValue()); //$NON-NLS-1$
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

      LoginDialog loginDialog = new LoginDialog(shell, certMgr);

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

         LoginJob job = new LoginJob(Display.getCurrent(), ignoreProtocolVersion);

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

         ProgressMonitorDialog monitorDialog = new ProgressMonitorDialog(null);
         try
         {
            monitorDialog.run(true, false, job);
            success = true;
         }
         catch(InvocationTargetException e)
         {
            logger.error("Login job failed", e.getCause());
            MessageDialog.openError(null, i18n.tr("Connection Error"), e.getCause().getLocalizedMessage());
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
   private String showPasswordRequestDialog(String title, String message)
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
    * @see org.eclipse.rap.rwt.client.service.StartupParameters#getParameterNames()
    */
   @Override
   public Collection<String> getParameterNames()
   {
      StartupParameters service = getClient().getService(StartupParameters.class);
      return service == null ? new ArrayList<String>() : service.getParameterNames();
   }

   /**
    * @see org.eclipse.rap.rwt.client.service.StartupParameters#getParameter(java.lang.String)
    */
   @Override
   public String getParameter(String name)
   {
      StartupParameters service = getClient().getService(StartupParameters.class);
      return service == null ? null : service.getParameter(name);
   }

   /**
    * @see org.eclipse.rap.rwt.client.service.StartupParameters#getParameterValues(java.lang.String)
    */
   @Override
   public List<String> getParameterValues(String name)
   {
      StartupParameters service = getClient().getService(StartupParameters.class);
      return service == null ? null : service.getParameterValues(name);
   }
}
