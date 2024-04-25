/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.base.login;

import java.lang.reflect.InvocationTargetException;
import java.security.Signature;
import java.security.cert.Certificate;
import java.util.List;
import java.util.Locale;
import java.util.ServiceLoader;
import java.util.UUID;
import org.apache.commons.codec.binary.Base64;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Display;
import org.netxms.base.NXCommon;
import org.netxms.base.VersionInfo;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.TwoFactorAuthenticationCallback;
import org.netxms.client.constants.AuthenticationType;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.ObjectCategory;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.services.LoginListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Login job
 */
public class LoginJob implements IRunnableWithProgress
{
   private I18n i18n = LocalizationHelper.getI18n(LoginJob.class);
   private Logger logger = LoggerFactory.getLogger(LoginJob.class);
   private Display display;
   private String server;
   private String loginName;
   private boolean enableCompression;
   private boolean ignoreProtocolVersion;
   private AuthenticationType authMethod;
   private String password;
   private Certificate certificate;
   private Signature signature;
   private String clientAddress;

   /**
    * @param display
    * @param server
    * @param loginName
    * @param encryptSession
    */
   public LoginJob(Display display, boolean ignoreProtocolVersion, boolean enableCompression)
   {
      this.display = display;

      PreferenceStore settings = PreferenceStore.getInstance(display);
      this.server = settings.getAsString("Connect.Server");
      this.loginName = settings.getAsString("Connect.Login");
      this.enableCompression = enableCompression;
      this.ignoreProtocolVersion = ignoreProtocolVersion;
      authMethod = AuthenticationType.PASSWORD;
      clientAddress = Registry.getClientAddress();
   }

   /**
    * @see org.eclipse.jface.operation.IRunnableWithProgress#run(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException
   {
      monitor.beginTask(i18n.tr("Connecting..."), 9);
      final String hostName;
      int port = NXCSession.DEFAULT_CONN_PORT;
      final String[] split = server.split(":");
      if (split.length == 2)
      {
         hostName = split[0];
         try
         {
            port = Integer.valueOf(split[1]);
         }
         catch(NumberFormatException e)
         {
            // ignore
         }
      }
      else
      {
         hostName = server;
      }

      logger.info("Connecting to " + hostName + " port " + port);

      final NXCSession session = createSession(hostName, port);
      try
      {
         session.setClientLanguage(Locale.getDefault().getLanguage());

         session.setClientInfo("nxmc/" + VersionInfo.version());
         session.setClientType(Registry.IS_WEB_CLIENT ? NXCSession.WEB_CLIENT : NXCSession.DESKTOP_CLIENT);
         session.setClientAddress(clientAddress);
         session.setIgnoreProtocolVersion(ignoreProtocolVersion);
         monitor.worked(1);

         session.connect(new int[] { ProtocolVersion.INDEX_FULL });
         monitor.worked(1);

         session.login(authMethod, loginName, password, certificate, signature, new TwoFactorAuthenticationCallback() {
            private boolean trustedDevice = false;

            @Override
            public int selectMethod(final List<String> methods)
            {
               if (methods.size() == 1)
                  return 0; // Skip selection dialog if only one method is available

               final int[] selection = new int[1];
               selection[0] = -1;
               display.syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     TwoFactorMetodSelectionDialog dlg = new TwoFactorMetodSelectionDialog(null, methods);
                     if (dlg.open() == Window.OK)
                        selection[0] = dlg.getSelectedMethod();
                  }
               });
               return selection[0];
            }

            @Override
            public String getUserResponse(final String challenge, final String qrLabel, final boolean trustedDevicesAllowed)
            {
               final String[] response = new String[1];
               display.syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     TwoFactorResponseDialog dlg = new TwoFactorResponseDialog(null, challenge, qrLabel, trustedDevicesAllowed);
                     if (dlg.open() == Window.OK)
                     {
                        response[0] = dlg.getResponse();
                        trustedDevice = dlg.isTrustedDevice();
                     }
                  }
               });
               return response[0];
            }

            @Override
            public void saveTrustedDeviceToken(long serverId, String username, byte[] token)
            {
               final String key = "TrustedDeviceToken." + Long.toString(serverId) + "." + username;
               display.syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (trustedDevice)
                        PreferenceStore.getInstance().set(key, Base64.encodeBase64String(token));
                     else
                        PreferenceStore.getInstance().remove(key);
                  }
               });
            }

            @Override
            public byte[] getTrustedDeviceToken(long serverId, String username)
            {
               final String key = "TrustedDeviceToken." + Long.toString(serverId) + "." + username;
               final String[] token = new String[1];
               display.syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     token[0] = PreferenceStore.getInstance().getAsString(key);
                  }
               });
               return ((token[0] != null) && !token[0].isEmpty()) ? Base64.decodeBase64(token[0]) : null;
            }
         });
         monitor.worked(1);

         monitor.setTaskName(i18n.tr("Synchronizing objects..."));
         PreferenceStore store = PreferenceStore.getInstance();
         boolean fullySync = store.getAsBoolean("Connect.FullObjectSync", false);
         session.syncObjects(fullySync);
         session.syncAssetManagementSchema();
         monitor.worked(1);

         monitor.setTaskName(i18n.tr("Synchronizing image library..."));
         ImageProvider imageProvider = ImageProvider.createInstance(display, session);
         for(ObjectCategory c : session.getObjectCategories())
         {
            UUID imageId = c.getIcon();
            if ((imageId != null) && !imageId.equals(NXCommon.EMPTY_GUID))
               imageProvider.preloadImageFromServer(imageId);
         }
         monitor.worked(1);

         monitor.setTaskName(i18n.tr("Synchronizing user database..."));
         session.subscribeToUserDBUpdates();
         monitor.worked(1);

         monitor.setTaskName(i18n.tr("Synchronizing events configuration..."));
         try
         {
            session.syncEventTemplates();
         }
         catch(NXCException e)
         {
            if (e.getErrorCode() != RCC.ACCESS_DENIED)
               throw e;
         }
         try
         {
            session.syncAlarmCategories();
         }
         catch(NXCException e)
         {
            if (e.getErrorCode() != RCC.ACCESS_DENIED)
               throw e;
         }
         monitor.worked(1);

         monitor.setTaskName(i18n.tr("Subscribing to notifications..."));
         session.subscribe(NXCSession.CHANNEL_ALARMS);
         monitor.worked(1);

         Registry.setSession(display, session);

         callLoginListeners(session);
         monitor.worked(1);

         setupSessionListener(session, display);

         logger.info("Creating keepalive timer");
         new KeepAliveTimer(session).start();
      }
      catch(Exception e)
      {
         session.disconnect();
         throw new InvocationTargetException(e);
      }
      finally
      {
         monitor.setTaskName(""); //$NON-NLS-1$
         monitor.done();
      }

      logger.info("Login job completed");
   }

   /**
    * Setup session listener
    * 
    * @param session
    * @param display
    */
   private static void setupSessionListener(final NXCSession session, final Display display)
   {
      session.addListener(new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getCode() == SessionNotification.SYSTEM_ACCESS_CHANGED)
            {
               // TODO: implement UI update
            }
         }
      });
   }

   /**
    * Create new session object.
    * 
    * @param hostName
    * @param port
    * @return
    */
   private NXCSession createSession(String hostName, int port)
   {
      // TODO: implement session providers
      return new NXCSession(hostName, port, enableCompression);
   }

   /**
    * Inform all registered login listeners about successful login
    * 
    * @param session new client session
    */
   private void callLoginListeners(NXCSession session)
   {
      ServiceLoader<LoginListener> loader = ServiceLoader.load(LoginListener.class, getClass().getClassLoader());
      for(LoginListener l : loader)
      {
         logger.debug("Calling login listener " + l.toString());
         try
         {
            l.afterLogin(session, display);
         }
         catch(Exception e)
         {
            logger.error("Exception in login listener", e);
         }
      }
   }

   /**
    * Set password for this login job
    * 
    * @param password
    */
   public void setPassword(String password)
   {
      this.password = password;
      authMethod = AuthenticationType.PASSWORD;
   }

   /**
    * Set certificate and signature for this login job
    * 
    * @param signature
    */
   public void setCertificate(Certificate certificate, Signature signature)
   {
      this.certificate = certificate;
      this.signature = signature;
      authMethod = AuthenticationType.CERTIFICATE;
   }

   /**
    * Set authentication mode to "token" (login name will be interpreted as token).
    */
   public void setAuthByToken()
   {
      authMethod = AuthenticationType.TOKEN;
   }

   /**
    * @param enableCompression the enableCompression to set
    */
   public void setEnableCompression(boolean enableCompression)
   {
      this.enableCompression = enableCompression;
   }
}
