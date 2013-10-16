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
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.operation.ModalContext;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.application.ActionBarAdvisor;
import org.eclipse.ui.application.IActionBarConfigurer;
import org.eclipse.ui.application.IWorkbenchWindowConfigurer;
import org.eclipse.ui.application.WorkbenchWindowAdvisor;
import org.netxms.api.client.Session;
import org.netxms.api.client.users.UserManager;
import org.netxms.client.NXCException;
import org.netxms.client.constants.RCC;
import org.netxms.ui.eclipse.console.dialogs.LoginDialog;
import org.netxms.ui.eclipse.console.dialogs.PasswordExpiredDialog;
import org.netxms.ui.eclipse.console.dialogs.SecurityWarningDialog;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Workbench window advisor
 */
public class NXMCWorkbenchWindowAdvisor extends WorkbenchWindowAdvisor
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
      Activator.getDefault().getStatusItemConnection().setImage(
            Activator.getImageDescriptor(session.isEncrypted() ? "icons/conn_encrypted.png" : "icons/conn_unencrypted.png").createImage());
      Activator.getDefault().getStatusItemConnection().setText(
            session.getUserName() + "@" + session.getServerAddress() + " (" + session.getServerVersion() + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$

      if (Activator.getDefault().getPreferenceStore().getBoolean("SHOW_TRAY_ICON")) //$NON-NLS-1$
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
      String password;

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
         }
         else if (s.equals("-auto")) //$NON-NLS-1$
         {
            autoConnect = true;
         }
      }

      boolean encrypt = true;
      LoginDialog loginDialog = new LoginDialog(null);

      while(!success)
      {
         if (!autoConnect)
         {
            showLoginDialog(loginDialog);
         }
         else
         {
            autoConnect = false; // only do auto connect first time
         }

         LoginJob job = new LoginJob(display, settings.get("Connect.Server"), //$NON-NLS-1$ 
               settings.get("Connect.Login"), //$NON-NLS-1$
               encrypt); //$NON-NLS-1$
         

         switch(loginDialog.getAuthenticationMethod()) 
         { 
            case AUTHENTICATION_PASSWORD:
               job.setPassword(loginDialog.getPassword());
               break;
 
            case AUTHENTICATION_CERTIFICATE: 
               job.setCertificate(loginDialog.getCertificate()); 
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

      // if (!success)
      // return;

      
      // TODO: refactor to not require this
      password = loginDialog.getPassword();
      
      // Suggest user to change password if it is expired
      final Session session = ConsoleSharedData.getSession();
      if (session.isPasswordExpired())
      {
         requestPasswordChange(password, session);
      }
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

      return SecurityWarningDialog.showSecurityWarning(
            null, String.format("NetXMS server %s does not support encryption. Do you want to connect anyway?", settings.get("Connect.Server")),
            "NetXMS server you are connecting to does not support encryption. If you countinue, information containing your credentials will be " +
            "sent in clear text and could easily be read by a third party.\n\n" +
            "For assistance, contact your network administrator or the owner of the NetXMS server.\n\n");
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
}
