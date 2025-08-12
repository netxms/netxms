/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
import java.io.UnsupportedEncodingException;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Locale;
import org.apache.commons.codec.binary.Base64;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.window.ApplicationWindow;
import org.eclipse.jface.window.Window;
import org.eclipse.jface.window.Window.IExceptionHandler;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.application.EntryPoint;
import org.eclipse.rap.rwt.client.service.ExitConfirmation;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.rap.rwt.client.service.StartupParameters;
import org.eclipse.rap.rwt.internal.application.ApplicationContextImpl;
import org.eclipse.rap.rwt.internal.service.ContextProvider;
import org.eclipse.rap.rwt.service.ServerPushSession;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.VersionInfo;
import org.netxms.certificate.manager.CertificateManagerProvider;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.base.UIElementFilter;
import org.netxms.nxmc.base.dialogs.PasswordExpiredDialog;
import org.netxms.nxmc.base.login.LoginDialog;
import org.netxms.nxmc.base.login.LoginJob;
import org.netxms.nxmc.base.login.LoginProgressDialog;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.base.windows.PopOutViewWindow;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.views.AdHocDashboardView;
import org.netxms.nxmc.modules.datacollection.SummaryTablesCache;
import org.netxms.nxmc.modules.datacollection.api.GraphTemplateCache;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.modules.logviewer.LogDescriptorRegistry;
import org.netxms.nxmc.modules.networkmaps.views.AdHocPredefinedMapView;
import org.netxms.nxmc.modules.objects.ObjectIcons;
import org.netxms.nxmc.modules.objecttools.ObjectToolsCache;
import org.netxms.nxmc.modules.snmp.shared.MibCache;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import jakarta.servlet.http.Cookie;

/**
 * Application startup code
 */
public class Startup implements EntryPoint, StartupParameters
{
   public static final String LOGIN_COOKIE_NAME = "nxmcSessionLogin";

   private static final Logger logger = LoggerFactory.getLogger(Startup.class);

   private I18n i18n = LocalizationHelper.getI18n(Startup.class);
   private Display display;
   private long activityTimestamp = System.currentTimeMillis();

   static
   {
      Window.setExceptionHandler(new IExceptionHandler() {
         @Override
         public void handleException(Throwable t)
         {
            if (t instanceof ThreadDeath)
            {
               // Don't catch ThreadDeath as this is a normal occurrence when UI thread is terminated
               throw (Error)t;
            }
            logger.error("Unhandled event loop exception", t);
         }
      });
   }

   /**
    * @see org.eclipse.rap.rwt.application.EntryPoint#createUI()
    */
   @Override
   public int createUI()
   {
      display = new Display();
      Thread.currentThread().setPriority(Math.min(Thread.MAX_PRIORITY, Thread.NORM_PRIORITY + 1));

      display.setData(RWT.CANCEL_KEYS, new String[] { "CTRL+E", "CTRL+F", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10" });
      display.setData(RWT.ACTIVE_KEYS, new String[] { "CTRL+E", "CTRL+F", "CTRL+F2", "F5", "F7", "F8", "F10" });

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
      String language = getParameter("lang");
      if ((language == null) || language.isEmpty())
         language = PreferenceStore.getInstance().getAsString("nxmc.language", "en");
      logger.info("Language: " + language);
      RWT.setLocale(Locale.forLanguageTag(language));

      DateFormatFactory.createInstance();
      SharedIcons.init();
      StatusDisplayInfo.init(display);
      ObjectIcons.init(display);
      DataCollectionDisplayInfo.init();

      boolean kioskMode = false;
      Window window = null;
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

         display.addListener(SWT.Dispose, (e) -> {
            logger.info("Main display disposed");
            NXCSession session = Registry.getSession(display);
            if (session != null)
               session.disconnect();
            Registry.dispose();
         });

         kioskMode = Boolean.parseBoolean(getParameter("kiosk-mode"));
         if (!kioskMode)
         {
            MainWindow w = new MainWindow();
            Registry.setMainWindow(w);
            window = w;
         }

         String dashboardId = getParameter("dashboard");
         logger.debug("Dashboard=" + dashboardId);
         if (dashboardId != null)
         {
            Dashboard dashboard = getObjectById(dashboardId, Dashboard.class);
            if (dashboard != null)
            {
               final AdHocDashboardView view = new AdHocDashboardView(0, dashboard, null);
               if (kioskMode)
               {
                  window = PopOutViewWindow.create(view);
               }
               else
               {
                  ((MainWindow)window).addPostOpenRunnable(() -> Display.getCurrent().asyncExec(() -> PopOutViewWindow.open(view)));
               }
            }
            else
            {
               logger.warn("Cannot find dashboard object with name or ID \"" + dashboardId + "\"");
            }
         }

         String mapId = getParameter("map");
         logger.debug("Map=" + mapId);
         if (mapId != null)
         {
            NetworkMap map = getObjectById(mapId, NetworkMap.class);
            if (map != null)
            {
               final AdHocPredefinedMapView view = new AdHocPredefinedMapView(map.getObjectId(), map);
               if (kioskMode)
               {
                  window = PopOutViewWindow.create(view);
               }
               else
               {
                  ((MainWindow)window).addPostOpenRunnable(() -> Display.getCurrent().asyncExec(() -> PopOutViewWindow.open(view)));
               }
            }
            else
            {
               logger.warn("Cannot find map object with name or ID \"" + mapId + "\"");
            }
         }

         if (window == null)
         {
            window = new ApplicationWindow(null) {
               @Override
               protected int getShellStyle()
               {
                  return SWT.NO_TRIM;
               }

               @Override
               protected void configureShell(Shell shell)
               {
                  super.configureShell(shell);
                  shell.setMaximized(true);
               }

               @Override
               protected Control createContents(Composite parent)
               {
                  Composite content = new Composite(parent, SWT.NONE);
                  content.setLayout(new GridLayout());
                  Label label = new Label(content, SWT.NONE);
                  label.setText("Invalid resource ID");
                  label.setFont(JFaceResources.getBannerFont());
                  return content;
               }
            };
         }

         window.setBlockOnOpen(true);
      }

      NXCSession session = Registry.getSession();
      MibCache.init(session, display);
      ObjectToolsCache.init();
      ObjectToolsCache.attachSession(display, session);
      SummaryTablesCache.attachSession(display, session);
      GraphTemplateCache.attachSession(display, session);
      LogDescriptorRegistry.attachSession(display, session);
      Registry.setSingleton(UIElementFilter.class, new UIElementFilter(session));

      if (!kioskMode)
      {
         ExitConfirmation exitConfirmation = RWT.getClient().getService(ExitConfirmation.class);
         exitConfirmation.setMessage(i18n.tr("This will terminate your current session. Are you sure?"));
      }

      session.addListener((n) -> processSessionNotification(n));

      ServerPushSession pushSession = new ServerPushSession();
      pushSession.start();

      if (popoutId == null)
         setInactivityHandler(session.getClientConfigurationHintAsInt("InactivityTimeout", 0));

      logger.debug("About to open {} window", (window instanceof MainWindow) ? "main" : "pop-out");
      window.open();

      pushSession.stop();

      logger.info("Application instance exit");
      display.dispose();

      return 0;
   }

   /**
    * Set session inactivity handler.
    *
    * @param display current display
    * @param inactivityTimeout inactivity timeout in seconds
    */
   private void setInactivityHandler(int inactivityTimeout)
   {
      if (inactivityTimeout <= 0)
      {
         logger.debug("User inactivity timer not set");
         return;
      }

      Listener activityListener = (e) -> {
         activityTimestamp = System.currentTimeMillis();
      };
      display.addFilter(SWT.MouseDown, activityListener);
      display.addFilter(SWT.MouseDoubleClick, activityListener);
      display.addFilter(SWT.KeyDown, activityListener);

      final long inactivityTimeoutMs = inactivityTimeout * 1000L;
      Thread thread = new Thread(() -> {
         logger.debug("User inactivity timer started");
         while(true)
         {
            try
            {
               Thread.sleep(10000);
            }
            catch(InterruptedException e)
            {
            }
            if (activityTimestamp < System.currentTimeMillis() - inactivityTimeoutMs)
            {
               logger.debug("User inactivity timer fired");
               Registry.getSession(display).disconnectInactiveSession();
               break;
            }
         }
      }, "InactivityTimer");
      thread.setDaemon(true);
      thread.start();
   }

   /**
    * Get object by ID
    * 
    * @param objectId numeric object ID or object name
    * @param objectClass object class
    */
   @SuppressWarnings("unchecked")
   private <T extends AbstractObject> T getObjectById(String objectId, Class<T> objectClass)
   {
      NXCSession session = Registry.getSession();
      try
      {
         long id = Long.parseLong(objectId);
         return session.findObjectById(id, objectClass);
      }
      catch(NumberFormatException e)
      {
         return (T)session.findObjectByName(objectId, (o) -> objectClass.isInstance(o));
      }
   }

   /**
    * Show login dialog and perform login
    */
   private boolean doLogin()
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      boolean success = false;
      boolean autoConnect = false;
      boolean tokenAuth = false;
      String password = "";

      String s = getParameter("login");
      if (s != null)
      {
         settings.set("Connect.Login", s);
      }

      s = getParameter("password");
      if (s != null)
      {
         password = s;
         settings.set("Connect.AuthMethod", AuthenticationType.PASSWORD.getValue());
      }

      s = getParameter("token");
      if (s != null)
      {
         settings.set("Connect.Login", s);
         tokenAuth = true;
      }

      AppPropertiesLoader appProperties = new AppPropertiesLoader();

      s = getParameter("auto");
      if (s != null)
      {
         autoConnect = true;
      }
      else if (appProperties.getPropertyAsBoolean("autoLoginOnReload", true))
      {
         String storedCredentials = getCredentialsFromCookie();
         if (storedCredentials != null)
         {
            String[] parts = storedCredentials.split("`", 2);
            if (parts.length == 2)
            {
               logger.debug("Using stored credentials");
               settings.set("Connect.Login", parts[0]);
               password = parts[1];
               autoConnect = true;
            }
         }
      }

      settings.set("Connect.Server", appProperties.getProperty("server", "127.0.0.1"));

      LoginDialog loginDialog = new LoginDialog(appProperties);
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

         boolean ignoreProtocolVersion = appProperties.getPropertyAsBoolean("ignoreProtocolVersion", false);
         boolean enableCompression = appProperties.getPropertyAsBoolean("enableCompression", true);
         LoginJob job = new LoginJob(display, ignoreProtocolVersion, enableCompression);
         if (tokenAuth)
         {
            tokenAuth = false;  // only do token auth for first time
            job.setAuthByToken();
         }
         else
         {
            job.setPassword(password);
         }

         LoginProgressDialog monitorDialog = new LoginProgressDialog(appProperties);
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
               loginDialog.setErrorMessage(cause.getLocalizedMessage());
            }
         }
         catch(Exception e)
         {
            logger.error("Unexpected exception while running login job", e);
            loginDialog.setErrorMessage(i18n.tr("Internal error: {0}", e.toString()));
         }
      }

      CertificateManagerProvider.dispose();

      // Suggest user to change password if it is expired
      final NXCSession session = Registry.getSession();
      if ((session.getAuthenticationMethod() == AuthenticationType.PASSWORD) && session.isPasswordExpired())
      {
         requestPasswordChange(loginDialog.getPassword(), session);
      }

      if (appProperties.getPropertyAsBoolean("autoLoginOnReload", true))
      {
         try
         {
            Cookie cookie = new Cookie(LOGIN_COOKIE_NAME, Base64.encodeBase64String((settings.getAsString("Connect.Login") + "`" + password).getBytes("UTF-8")));
            cookie.setSecure(ContextProvider.getRequest().isSecure());
            cookie.setMaxAge(-1);
            cookie.setHttpOnly(true);
            ContextProvider.getResponse().addCookie(cookie);
         }
         catch(UnsupportedEncodingException e)
         {
            logger.debug("Error encoding credentials cookie", e);
         }
      }

      return true;
   }

   /**
    * Get stored credentials from cookie
    *
    * @return store ID or null
    */
   private static String getCredentialsFromCookie()
   {
      Cookie[] cookies = ContextProvider.getRequest().getCookies();
      if (cookies != null)
      {
         for(int i = 0; i < cookies.length; i++)
         {
            Cookie cookie = cookies[i];
            if (LOGIN_COOKIE_NAME.equals(cookie.getName()))
            {
               logger.debug("Found credentials cookie: " + cookie.getValue());
               try
               {
                  return new String(Base64.decodeBase64(cookie.getValue()), "UTF-8");
               }
               catch(UnsupportedEncodingException e)
               {
                  return null;
               }
            }
         }
      }
      return null;
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
    * Process session notifications
    *
    * @param n session notification
    */
   private void processSessionNotification(SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.CONNECTION_BROKEN:
            processDisconnect(i18n.tr("communication error"));
            break;
         case SessionNotification.INACTIVITY_TIMEOUT:
            processDisconnect(i18n.tr("session terminated due to inactivity"));
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
   private void processDisconnect(String reason)
   {
      display.asyncExec(() -> {
         Shell shell = Registry.getMainWindowShell();
         MessageDialogHelper.openWarning(shell, i18n.tr("Disconnected"), i18n.tr("Connection with the server was lost ({0}). Application will now exit.", reason));

         prepareDisconnect();

         shell.dispose();

         JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
         executor.execute("window.location.reload();");
      });
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

   /**
    * Prepare session disconnect (disable exit confirmation, clear cookies, etc.)
    */
   public static void prepareDisconnect()
   {
      logger.debug("Preparing session disconnect");

      ExitConfirmation exitConfirmation = RWT.getClient().getService(ExitConfirmation.class);
      exitConfirmation.setMessage(null);

      logger.debug("Clearing cookies");
      Cookie cookie = new Cookie(Startup.LOGIN_COOKIE_NAME, "");
      cookie.setSecure(ContextProvider.getRequest().isSecure());
      cookie.setMaxAge(0);
      cookie.setHttpOnly(true);
      ContextProvider.getResponse().addCookie(cookie);
   }
}
