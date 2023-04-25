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

import static org.eclipse.rap.rwt.RWT.getClient;
import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import javax.servlet.http.HttpServletRequest;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.window.Window;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.application.EntryPoint;
import org.eclipse.rap.rwt.client.service.ExitConfirmation;
import org.eclipse.rap.rwt.client.service.StartupParameters;
import org.eclipse.rap.rwt.internal.application.ApplicationContextImpl;
import org.eclipse.swt.widgets.Display;
import org.netxms.base.VersionInfo;
import org.netxms.certificate.manager.CertificateManagerProvider;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.client.constants.RCC;
import org.netxms.nxmc.base.dialogs.PasswordExpiredDialog;
import org.netxms.nxmc.base.login.LoginDialog;
import org.netxms.nxmc.base.login.LoginJob;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.AlarmNotifier;
import org.netxms.nxmc.modules.datacollection.SummaryTablesCache;
import org.netxms.nxmc.modules.datacollection.api.GraphTemplateCache;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.modules.objects.ObjectIcons;
import org.netxms.nxmc.modules.objecttools.ObjectToolsCache;
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

   /**
    * @see org.eclipse.rap.rwt.application.EntryPoint#createUI()
    */
   @Override
   public int createUI()
   {
      display = new Display();

      display.setData(RWT.CANCEL_KEYS, new String[] { "CTRL+F", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10" });
      display.setData(RWT.ACTIVE_KEYS, new String[] { "CTRL+F", "CTRL+F2", "F5", "F7", "F8", "F10" });

      File tempDir = (File)RWT.getUISession().getHttpSession().getServletContext().getAttribute("javax.servlet.context.tempdir");
      File stateDir = new File(tempDir + File.separator + "state");
      if (!stateDir.isDirectory())
      {
         stateDir.mkdir();
      }
      Registry.setStateDir(stateDir);

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
      DateFormatFactory.createInstance();
      SharedIcons.init();
      StatusDisplayInfo.init(display);
      ObjectIcons.init(display);
      DataCollectionDisplayInfo.init();

      Window window;
      String popoutId = getParameter("pop-out-id");
      if (popoutId != null)
      {
         window = PopOutViewWindow.create(popoutId);
         if (window == null)
         {
            logger.info("Application instance exit (invalid pop-out request ID)");
            display.dispose();
            return 0;
         }
      }
      else
      {
         if (!doLogin())
         {
            logger.info("Application instance exit (cannot login)");
            display.dispose();
            return 0;
         }

         MainWindow w = new MainWindow();
         Registry.setMainWindow(w);
         w.setBlockOnOpen(true);
         window = w;
      }

      NXCSession session = Registry.getSession();
      MibCache.init(session, display);
      ObjectToolsCache.init();
      ObjectToolsCache.attachSession(display, session);
      SummaryTablesCache.attachSession(display, session);
      GraphTemplateCache.attachSession(display, session);

      ExitConfirmation exitConfirmation = RWT.getClient().getService(ExitConfirmation.class);
      exitConfirmation.setMessage(i18n.tr("This will terminate your current session. Are you sure?"));

      window.open();

      logger.info("Application instance exit");
      AlarmNotifier.stop();
      display.dispose();

      return 0;
   }

   /**
    * Show login dialog and perform login
    */
   private boolean doLogin()
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      boolean success = false;
      boolean autoConnect = false;
      boolean ignoreProtocolVersion = false;
      String password = "";

      HttpServletRequest request = RWT.getRequest();
      String s = request.getParameter("login");
      if (s != null)
      {
         settings.set("Connect.Login", s);
      }

      s = request.getParameter("password");
      if (s != null)
      {
         password = s;
         settings.set("Connect.AuthMethod", AuthenticationType.PASSWORD.getValue());
      }

      s = request.getParameter("auto");
      if (s != null)
      {
         autoConnect = true;
      }

      AppPropertiesLoader appProperties = new AppPropertiesLoader();
      settings.set("Connect.Server", appProperties.getProperty("server", "127.0.0.1"));

      LoginDialog loginDialog = new LoginDialog(null, appProperties);
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
         job.setPassword(password);

         ProgressMonitorDialog monitorDialog = new ProgressMonitorDialog(null);
         try
         {
            monitorDialog.run(true, false, job);
            success = true;
         }
         catch(InvocationTargetException e)
         {
            Throwable cause = e.getCause();
            logger.error("Login job failed", cause);
            if (!(cause instanceof NXCException) || (((NXCException)cause).getErrorCode() != RCC.OPERATION_CANCELLED))
            {
               MessageDialog.openError(null, i18n.tr("Connection Error"), cause.getLocalizedMessage());
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
