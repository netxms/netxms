/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.operation.ModalContext;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.netxms.api.client.Session;
import org.netxms.api.client.users.UserManager;
import org.netxms.certificate.loader.KeyStoreRequestListener;
import org.netxms.certificate.manager.CertificateManager;
import org.netxms.certificate.manager.CertificateManagerProvider;
import org.netxms.certificate.manager.exception.SignatureImpossibleException;
import org.netxms.certificate.request.KeyStoreEntryPasswordRequestListener;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.ui.eclipse.console.dialogs.LoginDialog;
import org.netxms.ui.eclipse.console.dialogs.PasswordExpiredDialog;
import org.netxms.ui.eclipse.console.dialogs.PasswordRequestDialog;
import org.netxms.ui.eclipse.console.dialogs.SecurityWarningDialog;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Workbench window advisor
 */
public class NXMCWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor implements KeyStoreRequestListener, KeyStoreEntryPasswordRequestListener
{
   /**
    * @param configurer
    */
   public NXMCWorkbenchWindowAdvisor(IWorkbenchWindowConfigurer configurer)
   {
      super(configurer);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.application.WorkbenchWindowAdvisor#createActionBarAdvisor(org.eclipse.ui.application.IActionBarConfigurer)
    */
   @Override
   public ActionBarAdvisor createActionBarAdvisor(IActionBarConfigurer configurer)
   {
      return new NXMCActionBarAdvisor(configurer);
   }

   /*
    * (non-Javadoc)
    * 
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
      configurer.setShowPerspectiveBar(true);

      TweakletManager.preWindowOpen(configurer);
   }

   /**
    * Overridden to maximize the window when shown.
    */
   @Override
   public void postWindowCreate()
   {
      IWorkbenchWindowConfigurer configurer = getWindowConfigurer();

      Session session = ConsoleSharedData.getSession();
      Activator activator = Activator.getDefault();
      StatusLineContributionItem statusItemConnection = activator.getStatusItemConnection();
      statusItemConnection.setImage(Activator.getImageDescriptor(
            session.isEncrypted() ? "icons/conn_encrypted.png" : "icons/conn_unencrypted.png").createImage());
      statusItemConnection.setText(session.getUserName()
            + "@" + session.getServerAddress() + " (" + session.getServerVersion() + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$

      if (activator.getPreferenceStore().getBoolean("SHOW_TRAY_ICON")) //$NON-NLS-1$
         Activator.showTrayIcon();

      TweakletManager.postWindowCreate(configurer);
   }

   /**
    * Show login dialog and perform login
    */
   private void doLogin(final Display display)
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      boolean success = false;
      boolean autoConnect = false;
      String password = "";

      CertificateManager certMgr = CertificateManagerProvider.provideCertificateManager();
      certMgr.setKeyStoreRequestListener(this);
      certMgr.setEntryListener(this);

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
            settings.put("Connect.AuthMethod", NXCSession.AUTH_TYPE_PASSWORD);
         }
         else if (s.equals("-auto")) //$NON-NLS-1$
         {
            autoConnect = true;
         }
      }

      boolean encrypt = true;
      LoginDialog loginDialog = new LoginDialog(null, certMgr);

      while(!success)
      {
         if (!autoConnect)
         {
            showLoginDialog(loginDialog);
            password = loginDialog.getPassword();
         }
         else
         {
            autoConnect = false; // only do auto connect first time
         }

         LoginJob job = new LoginJob(display, 
         		settings.get("Connect.Server"), //$NON-NLS-1$ 
               settings.get("Connect.Login"), //$NON-NLS-1$
               encrypt);

         int authMethod;
         try
         {
         	authMethod = settings.getInt("Connect.AuthMethod"); //$NON-NLS-1$
         }
         catch(NumberFormatException e)
         {
         	authMethod = NXCSession.AUTH_TYPE_PASSWORD;
         }
         switch(authMethod)
         {
            case NXCSession.AUTH_TYPE_PASSWORD:
               job.setPassword(password);
               break;
            case NXCSession.AUTH_TYPE_CERTIFICATE:
               job.setSignature(getSignature(certMgr, loginDialog.getCertificate()));
               break;
         }

         try
         {
            ModalContext.run(job, true, SplashHandler.getInstance().getBundleProgressMonitor(), Display.getCurrent());
            success = true;
         }
         catch(InvocationTargetException e)
         {
            if ((e.getCause() instanceof NXCException)
                  && (((NXCException)e.getCause()).getErrorCode() == RCC.NO_ENCRYPTION_SUPPORT) && encrypt)
            {
               boolean alwaysAllow = settings.getBoolean("Connect.AllowUnencrypted." + settings.get("Connect.Server"));
               int action = getAction(settings, alwaysAllow);
               if (action != SecurityWarningDialog.NO)
               {
                  autoConnect = true;
                  encrypt = false;
                  if (action == SecurityWarningDialog.ALWAYS)
                  {
                     settings.put("Connect.AllowUnencrypted." + settings.get("Connect.Server"), true);
                  }
               }
            }
            else
            {
               e.getCause().printStackTrace();
               MessageDialog.openError(null, Messages.NXMCWorkbenchWindowAdvisor_connectionError, e.getCause()
                     .getLocalizedMessage()); //$NON-NLS-1$
            }
         }
         catch(Exception e)
         {
            e.printStackTrace();
            MessageDialog.openError(null, Messages.NXMCWorkbenchWindowAdvisor_exception, e.toString()); //$NON-NLS-1$
         }
      }

      CertificateManagerProvider.dispose();

      // Suggest user to change password if it is expired
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      if ((session.getAuthType() == NXCSession.AUTH_TYPE_PASSWORD) && session.isPasswordExpired())
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
      catch(SignatureImpossibleException sie)
      {
         return null;
      }

      return sign;
   }

   /**
    * @param settings
    * @param alwaysAllow
    * @return
    */
   private int getAction(IDialogSettings settings, boolean alwaysAllow)
   {
      if (alwaysAllow)
         return SecurityWarningDialog.YES;

      return SecurityWarningDialog
            .showSecurityWarning(
                  null,
                  String.format("NetXMS server %s does not support encryption. Do you want to connect anyway?",
                        settings.get("Connect.Server")),
                  "NetXMS server you are connecting to does not support encryption. If you countinue, information containing your credentials will be "
                        + "sent in clear text and could easily be read by a third party.\n\n"
                        + "For assistance, contact your network administrator or the owner of the NetXMS server.\n\n");
   }

   /**
    * 
    * @param dialog
    */
   private void showLoginDialog(LoginDialog dialog)
   {
      if (dialog.open() != Window.OK)
         System.exit(0);
   }

   /**
    * @param currentPassword
    * @param session
    */
   private void requestPasswordChange(final String currentPassword, final Session session)
   {
      final PasswordExpiredDialog dlg = new PasswordExpiredDialog(null);

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
                  monitor.setTaskName(Messages.NXMCWorkbenchWindowAdvisor_ChangingPassword);
                  ((UserManager)session).setUserPassword(session.getUserId(), dlg.getPassword(), currentPassword);
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
            MessageDialog.openInformation(null, Messages.NXMCWorkbenchWindowAdvisor_title_information,
                  Messages.NXMCWorkbenchWindowAdvisor_passwd_changed);
            return;
         }
         catch(InvocationTargetException e)
         {
            MessageDialog.openError(null, Messages.NXMCWorkbenchWindowAdvisor_title_error,
                  Messages.NXMCWorkbenchWindowAdvisor_cannot_change_passwd + " " + e.getCause().getLocalizedMessage()); //$NON-NLS-1$
         }
         catch(InterruptedException e)
         {
            MessageDialog.openError(null, Messages.NXMCWorkbenchWindowAdvisor_exception, e.toString());
         }
      }
   }

   @Override
   public String keyStoreLocationRequested()
   {
      Shell shell = Display.getCurrent().getActiveShell();

      FileDialog dialog = new FileDialog(shell);
      dialog.setText("Path to the certificate store");
      dialog.setFilterExtensions(new String[] { "*.p12; *.pfx" });
      dialog.setFilterNames(new String[] { "PKCS12 file (*.p12, *.pfx)" });

      return dialog.open();
   }

   @Override
   public String keyStorePasswordRequested()
   {
      return showPasswordRequestDialog("Certificate store password",
            "The selected store is password-protected, please provide the password.");
   }

   @Override
   public String keyStoreEntryPasswordRequested()
   {
      return showPasswordRequestDialog("Certificate password",
            "The selected certificate is password-protected, please provide the password.");
   }

   private String showPasswordRequestDialog(String title, String message)
   {
      Shell shell = Display.getCurrent().getActiveShell();

      PasswordRequestDialog dialog = new PasswordRequestDialog(shell);
      dialog.setTitle(title);
      dialog.setMessage(message);
      
      if(dialog.open() == Window.OK)
      {
         return dialog.getPassword();
      }
      
      return null;
   }
}
